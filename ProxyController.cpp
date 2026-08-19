// ProxyController.cc
#include "ProxyController.h"

#include <drogon/HttpClient.h>
#include <drogon/utils/Utilities.h>
#include <json/json.h> // JsonCpp
#include <regex>
#include <sstream>
#include <thread>
#include <iostream>
#include <openssl/ssl.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
using namespace drogon;
// helper: trim + sanitize host
static std::string trimStr(const std::string& s) {
	size_t a = s.find_first_not_of(" \t\r\n");
	if (a == std::string::npos) return "";
	size_t b = s.find_last_not_of(" \t\r\n");
	return s.substr(a, b - a + 1);
}
static std::string sanitizeHost(const std::string& raw) {
	std::string h = trimStr(raw);
	if (h.rfind("http://", 0) == 0) h = h.substr(7);
	else if (h.rfind("https://", 0) == 0) h = h.substr(8);
	auto pos = h.find('/');
	if (pos != std::string::npos) h = h.substr(0, pos);
	// remove port suffix if user accidentally included ":port"
	auto colon = h.find(':');
	if (colon != std::string::npos) h = h.substr(0, colon);
	return h;
}

// Resolve host to an IP string (first result). Returns "" on failure.
static std::string resolveHostToIp(const std::string& host, unsigned short port) {
	if (host.empty()) return "";
#ifdef _WIN32
	static bool wsaInited = false;
	if (!wsaInited) {
		WSADATA wd;
		if (WSAStartup(MAKEWORD(2, 2), &wd) != 0) {
			std::cerr << "[resolve] WSAStartup failed\n";
			return "";
		}
		wsaInited = true;
	}
#endif

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;
	char portStr[16];
	snprintf(portStr, sizeof(portStr), "%u", (unsigned)port);

	struct addrinfo* res = nullptr;
	int rc = getaddrinfo(host.c_str(), portStr, &hints, &res);
	if (rc != 0) {
#ifdef _WIN32
		std::cerr << "[resolve] getaddrinfo failed: " << rc << "\n";
#else
		std::cerr << "[resolve] getaddrinfo failed: " << gai_strerror(rc) << "\n";
#endif
		return "";
	}

	char ipbuf[INET6_ADDRSTRLEN] = { 0 };
	for (struct addrinfo* p = res; p != nullptr; p = p->ai_next) {
		void* addrPtr = nullptr;
		if (p->ai_family == AF_INET) {
			struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
			addrPtr = &(ipv4->sin_addr);
		}
		else if (p->ai_family == AF_INET6) {
			struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
			addrPtr = &(ipv6->sin6_addr);
		}
		else continue;
		if (inet_ntop(p->ai_family, addrPtr, ipbuf, sizeof(ipbuf))) {
			std::string ipstr(ipbuf);
			freeaddrinfo(res);
			return ipstr;
		}
	}
	freeaddrinfo(res);
	return "";
}

static const std::string API_KEY = "ReplaceWithStrongToken";

// simple API key check: header x-api-key or query param api_key
static bool checkApiKey(const HttpRequestPtr& req) {
	auto headerVal = req->getHeader("x-api-key");
	if (!headerVal.empty() && headerVal == API_KEY) return true;
	auto q = req->getParameter("api_key");
	return q == API_KEY;
}
//手动解析URL
static const std::tuple<std::string, std::string, uint16_t, std::string, std::string>
parseUrlManual(const std::string& url)
{
	std::string scheme = "http";  // 默认协议：http
	std::string host, portStr, path = "/", query;
	uint16_t port = 80;           // 默认端口：80（http）

	// 1. 匹配协议（http://、https://、ws://、wss://）
	std::regex schemeRegex(R"(^(\w+)://)");
	std::smatch schemeMatch;
	if (std::regex_search(url, schemeMatch, schemeRegex)) {
		scheme = schemeMatch[1];
		// 更新默认端口（根据协议）
		if (scheme == "https" || scheme == "wss") {
			port = 443;
		}
		else if (scheme == "ws") {
			port = 80;
		}
	}

	// 2. 截取协议后的部分（host:port/path?query）
	size_t schemeEnd = url.find("://");
	std::string rest = (schemeEnd != std::string::npos) ? url.substr(schemeEnd + 3) : url;

	// 3. 分割“主机+端口”和“路径+查询参数”（以 '/' 或 '?' 为界）
	size_t pathStart = rest.find_first_of("/?");
	std::string hostPortPart = (pathStart != std::string::npos) ? rest.substr(0, pathStart) : rest;
	std::string pathQueryPart = (pathStart != std::string::npos) ? rest.substr(pathStart) : "";

	// 4. 解析“主机+端口”（以 ':' 为界）
	size_t portStart = hostPortPart.find(":");
	if (portStart != std::string::npos) {
		host = hostPortPart.substr(0, portStart);
		portStr = hostPortPart.substr(portStart + 1);
		// 端口转换为数字（避免非法端口）
		try {
			port = static_cast<uint16_t>(std::stoi(portStr));
		}
		catch (...) {
			port = (scheme == "https") ? 443 : 80;  // 非法端口时用默认值
		}
	}
	else {
		host = hostPortPart;  // 没有端口，直接取主机名
	}

	// 5. 解析“路径+查询参数”（以 '?' 为界）
	size_t queryStart = pathQueryPart.find("?");
	if (queryStart != std::string::npos) {
		path = pathQueryPart.substr(0, queryStart);
		query = pathQueryPart.substr(queryStart + 1);
	}
	else {
		path = pathQueryPart.empty() ? "/" : pathQueryPart;  // 没有路径时默认为 '/'
	}

	return { scheme, host, port, path, query };

}
static HttpMethod stringToMethod(const std::string& m) {
	std::string s = m;
	for (auto& c : s) c = std::toupper(static_cast<unsigned char>(c));
	if (s == "GET") return drogon::Get;
	if (s == "POST") return drogon::Post;
	if (s == "PUT") return drogon::Put;
	if (s == "DELETE") return drogon::Delete;
	if (s == "HEAD") return drogon::Head;
	if (s == "OPTIONS") return drogon::Options;
	if (s == "PATCH") return drogon::Patch;
	return drogon::Get;
}

// makeAbsolute: make link absolute using base (very naive)
static std::string makeAbsolute(const std::string& base, const std::string& link) {
	if (link.empty()) return "";
	if (link.rfind("http://", 0) == 0 || link.rfind("https://", 0) == 0) return link;
	if (link.rfind("//", 0) == 0) {
		// preserve scheme from base
		std::regex re("^([a-zA-Z]+):");
		std::smatch m;
		if (std::regex_search(base, m, re)) {
			return m[1].str() + ":" + link;
		}
		else {
			return "https:" + link;
		}
	}

	// parse base using HttpURL
	auto [schemeParam, hostParam, portParam, pathParam, queryParam] = parseUrlManual(base);
	std::string scheme = schemeParam;
	std::string host = hostParam;
	unsigned short port = portParam;
	std::string basePath = pathParam;
	std::ostringstream os;
	os << scheme << "://" << host;
	if (port != 0 && port != 80 && port != 443) os << ":" << port;

	if (!link.empty() && link[0] == '/') {
		os << link;
		return os.str();
	}
	else 
	{
		// resolve relative path (remove last segment)
		auto idx = basePath.find_last_of('/');
		std::string prefix = (idx == std::string::npos) ? "/" : basePath.substr(0, idx + 1);
		os << prefix << link;
		return os.str();
	}
}

// POST /fetch
void ProxyController::fetch(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
	if (!checkApiKey(req)) {
		auto resp = HttpResponse::newHttpResponse();
		resp->setStatusCode(k401Unauthorized);
		resp->setBody("Unauthorized");
		callback(resp);
		return;
	}

	// parse JSON body using JsonCpp
	Json::CharReaderBuilder rbuilder;
	std::string errs;
	Json::Value jroot;
	std::string body(req->getBody());
	std::istringstream is(body);
	if (!Json::parseFromStream(rbuilder, is, &jroot, &errs)) {
		auto resp = HttpResponse::newHttpResponse();
		resp->setStatusCode(k400BadRequest);
		resp->setBody(std::string("invalid json: ") + errs);
		callback(resp);
		return;
	}

	if (!jroot.isMember("url") || !jroot["url"].isString()) {
		auto resp = HttpResponse::newHttpResponse();
		resp->setStatusCode(k400BadRequest);
		resp->setBody("missing url");
		callback(resp);
		return;
	}
	const std::string targetUrl = jroot["url"].asString();
	const std::string methodStr = jroot.isMember("method") && jroot["method"].isString() ? jroot["method"].asString() : "GET";
	const HttpMethod method = stringToMethod(methodStr);

	// headers
	std::map<std::string, std::string> headers;
	if (jroot.isMember("headers") && jroot["headers"].isObject()) {
		for (const auto& k : jroot["headers"].getMemberNames()) {
			if (jroot["headers"][k].isString()) headers[k] = jroot["headers"][k].asString();
		}
	}

	// optional body (base64)
	std::string bodyData;
	if (jroot.isMember("body") && jroot["body"].isString() && !jroot["body"].asString().empty()) {
		bodyData = utils::base64Decode(jroot["body"].asString());
	}

	// parse target url with HttpURL
	auto [schemeParam, hostParam, portParam, pathParam, queryParam] = parseUrlManual(targetUrl);

	std::string host = hostParam;
	unsigned short port = portParam;
	std::string scheme = schemeParam;
	std::string path = pathParam;
	if (!queryParam.empty()) path += "?" + queryParam;

	bool useSSL = (scheme == "https" || scheme == "wss");
	if (port == 0) port = useSSL ? 443 : 80;

	// create client and request
	auto client = HttpClient::newHttpClient(host, port, useSSL);
	auto remoteReq = HttpRequest::newHttpRequest();
	remoteReq->setMethod(method);
	remoteReq->setPath(path);
	// add headers (note: host/connection related headers might be overridden)
	for (auto& h : headers) {
		remoteReq->addHeader(h.first, h.second);
	}
	if (!bodyData.empty()) remoteReq->setBody(bodyData);

	// move callback into thread
	auto clientCopy = client;
	auto requestCopy = remoteReq;
	auto cb = std::move(callback);

	auto extractHostPortScheme = [](const std::string& url, std::string& outHost, unsigned short& outPort, std::string& outScheme) {
		std::regex re(R"(^([a-zA-Z][a-zA-Z0-9+\-.]*):\/\/([^\/:\?#]+)(?::(\d+))?)");
		std::smatch m;
		outHost.clear(); outPort = 0; outScheme.clear();
		if (std::regex_search(url, m, re)) {
			outScheme = m[1].str();
			outHost = m[2].str();
			if (m.size() > 3 && m[3].matched) {
				try { outPort = static_cast<unsigned short>(std::stoi(m[3].str())); }
				catch (...) { outPort = 0; }
			}
		}
		else {
			// treat url as bare host
			outHost = url;
		}
		};

	std::thread([=]() mutable {
		try {
			// extract host/port/scheme
			std::string rawHost;
			unsigned short port = 0;
			std::string scheme;
			extractHostPortScheme(targetUrl, rawHost, port, scheme);
			std::string host = sanitizeHost(rawHost);
			if (host.empty()) {
				auto r = HttpResponse::newHttpResponse();
				r->setStatusCode(HttpStatusCode::k400BadRequest);
				r->setBody("invalid upstream host");
				cb(r);
				return;
			}
			if (port == 0) port = (scheme == "https") ? 443 : 80;
			bool useSSL = (scheme == "https");
			std::cerr << "[proxy] upstream host='" << host << "' port=" << port << " ssl=" << useSSL << "\n";

			// Try direct resolve
			std::string ip = resolveHostToIp(host, port);
			HttpClientPtr client = nullptr;

			if (!ip.empty()) {
				std::cerr << "[proxy] resolved " << host << " -> " << ip << "\n";
				// create client by IP; will set Host header later
				client = HttpClient::newHttpClient(ip, port, useSSL);
				if (!client) {
					std::cerr << "[proxy] newHttpClient(ip) returned nullptr, fallback to hostname\n";
				}
				else {
					std::cerr << "[proxy] newHttpClient(ip) OK\n";
				}
			}

			if (!client) {
				// fallback to host
				client = HttpClient::newHttpClient(host, port, useSSL);
				if (!client) {
					std::cerr << "[proxy] newHttpClient(host) returned nullptr\n";
					auto r = HttpResponse::newHttpResponse();
					r->setStatusCode(HttpStatusCode::k502BadGateway);
					r->setBody("create upstream client failed");
					cb(r);
					return;
				}
				std::cerr << "[proxy] newHttpClient(host) OK\n";
			}

			// IMPORTANT: if we connected by IP, preserve original Host header for vhost
			if (requestCopy && !host.empty()) {
				requestCopy->addHeader("Host", host);
				// Also add a User-Agent to be safe
				requestCopy->addHeader("User-Agent", "Mozilla/5.0 (compatible; proxy/1.0)");
			}

			// send (synchronous) request with timeout
			double timeoutSeconds = 15.0;
			auto pr = client->sendRequest(requestCopy, timeoutSeconds);
			ReqResult result = pr.first;
			HttpResponsePtr upstreamResp = pr.second;

			std::cerr << "[proxy] sendRequest result=" << static_cast<int>(result) << " textual=" << result << "\n";

			if (!upstreamResp) {
				// If result indicates BadServerAddress, include debug info
				auto r = HttpResponse::newHttpResponse();
				r->setStatusCode(HttpStatusCode::k502BadGateway);
				std::string erro = "upstream request failed: " + std::to_string((int)result);
				r->setBody(erro);
				cb(r);
				return;
			}

			// downstream handling (你的原逻辑：HTML rewrite or passthrough)
			std::string body(upstreamResp->getBody());
			std::string contentType = upstreamResp->getHeader("content-type");
			if (!contentType.empty() && (contentType.find("text/html") != std::string::npos ||
				contentType.find("application/xhtml+xml") != std::string::npos)) {
				// ... (保留你的 HTML rewrite 逻辑)
				std::string s = body;
				// 这里可以把你的 regex rewrite 调用进来
				auto resp = HttpResponse::newHttpResponse();
				resp->addHeader("Content-Type", contentType);
				resp->setBody(s);
				cb(resp);
				return;
			}
			else {
				auto resp = HttpResponse::newHttpResponse();
				if (!contentType.empty()) resp->addHeader("Content-Type", contentType);
				resp->setBody(body);
				cb(resp);
				return;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[proxy] exception: " << e.what() << "\n";
			auto r = HttpResponse::newHttpResponse();
			r->setStatusCode(HttpStatusCode::k500InternalServerError);
			r->setBody(std::string("internal error: ") + e.what());
			cb(r);
			return;
		}
		catch (...) {
			std::cerr << "[proxy] unknown exception\n";
			auto r = HttpResponse::newHttpResponse();
			r->setStatusCode(HttpStatusCode::k500InternalServerError);
			r->setBody("internal unknown error");
			cb(r);
			return;
		}
		}).detach();
}

// GET /proxy?url=...
void ProxyController::proxyGet(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
	if (!checkApiKey(req)) {
		auto resp = HttpResponse::newHttpResponse();
		resp->setStatusCode(k401Unauthorized);
		resp->setBody("Unauthorized");
		callback(resp);
		return;
	}

	auto urlParam = req->getParameter("url");
	if (urlParam.empty()) {
		auto resp = HttpResponse::newHttpResponse();
		resp->setStatusCode(k400BadRequest);
		resp->setBody("missing url param");
		callback(resp);
		return;
	}
	std::string target = urlParam;
	auto [schemeParam, hostParam, portParam, pathParam, queryParam] = parseUrlManual(target);

	std::string host = hostParam;
	unsigned short port = portParam;
	std::string scheme = schemeParam;
	std::string path = pathParam;

	if (!queryParam.empty()) path += "?" + queryParam;
	bool useSSL = (scheme == "https" || scheme == "wss");
	if (port == 0) port = useSSL ? 443 : 80;
	auto client = HttpClient::newHttpClient(host, port, useSSL);
	auto remoteReq = HttpRequest::newHttpRequest();
	remoteReq->setMethod(drogon::Get);
	remoteReq->setPath(path);
	// move things into thread to call synchronous sendRequest
	auto clientCopy = client;
	auto requestCopy = remoteReq;
	std::string targetCopy = target;
	auto cb = std::move(callback);

	auto extractHostPortScheme = [](const std::string& url, std::string& outHost, unsigned short& outPort, std::string& outScheme) {
		std::regex re(R"(^([a-zA-Z][a-zA-Z0-9+\-.]*):\/\/([^\/:\?#]+)(?::(\d+))?)");
		std::smatch m;
		outHost.clear(); outPort = 0; outScheme.clear();
		if (std::regex_search(url, m, re)) {
			outScheme = m[1].str();
			outHost = m[2].str();
			if (m.size() > 3 && m[3].matched) {
				try { outPort = static_cast<unsigned short>(std::stoi(m[3].str())); }
				catch (...) { outPort = 0; }
			}
		}
		else {
			// treat url as bare host
			outHost = url;
		}
		};

	std::thread([=]() mutable {
		try {
			// extract host/port/scheme
			std::string rawHost;
			unsigned short port = 0;
			std::string scheme;
			extractHostPortScheme(targetCopy, rawHost, port, scheme);
			std::string host = sanitizeHost(rawHost);
			if (host.empty()) {
				auto r = HttpResponse::newHttpResponse();
				r->setStatusCode(HttpStatusCode::k400BadRequest);
				r->setBody("invalid upstream host");
				cb(r);
				return;
			}
			if (port == 0) port = (scheme == "https") ? 443 : 80;
			bool useSSL = (scheme == "https");
			std::cerr << "[proxy] upstream host='" << host << "' port=" << port << " ssl=" << useSSL << "\n";

			// Try direct resolve
			std::string ip = resolveHostToIp(host, port);
			HttpClientPtr client = nullptr;
			//使用IP访问
			if (!ip.empty()) {
				std::cerr << "[proxy] resolved " << host << " -> " << ip << "\n";
				// create client by IP; will set Host header later
				client = HttpClient::newHttpClient(ip, port, useSSL);
				if (!client) {
					std::cerr << "[proxy] newHttpClient(ip) returned nullptr, fallback to hostname\n";
				}
				else {
					std::cerr << "[proxy] newHttpClient(ip) OK\n";
				}
			}
			//使用host访问
			if (!client) {
				// fallback to host
				client = HttpClient::newHttpClient(host, port, useSSL);
				if (!client) {
					std::cerr << "[proxy] newHttpClient(host) returned nullptr\n";
					auto r = HttpResponse::newHttpResponse();
					r->setStatusCode(HttpStatusCode::k502BadGateway);
					r->setBody("create upstream client failed");
					cb(r);
					return;
				}
				std::cerr << "[proxy] newHttpClient(host) OK\n";
			}

			// IMPORTANT: if we connected by IP, preserve original Host header for vhost
			if (requestCopy && !host.empty()) {
				requestCopy->addHeader("Host", host);
				// Also add a User-Agent to be safe
				requestCopy->addHeader("User-Agent", "Mozilla/5.0 (compatible; proxy/1.0)");
			}

			// send (synchronous) request with timeout
			double timeoutSeconds = 15.0;


			auto pr = client->sendRequest(requestCopy, timeoutSeconds);
			ReqResult result = pr.first;
			HttpResponsePtr upstreamResp = pr.second;

			std::cerr << "[proxy] sendRequest result=" << static_cast<int>(result) << " textual=" << result << "\n";

			if (!upstreamResp) {
				// If result indicates BadServerAddress, include debug info
				auto r = HttpResponse::newHttpResponse();
				r->setStatusCode(HttpStatusCode::k502BadGateway);
				std::string erro = "upstream request failed: " + std::to_string((int)result);
				r->setBody(erro);
				cb(r);
				return;
			}

			// downstream handling (你的原逻辑：HTML rewrite or passthrough)
			std::string body(upstreamResp->getBody());
			std::string contentType = upstreamResp->getHeader("content-type");
			if (!contentType.empty() && (contentType.find("text/html") != std::string::npos ||
				contentType.find("application/xhtml+xml") != std::string::npos)) {
				// ... (保留你的 HTML rewrite 逻辑)
				std::string s = body;
				// 这里可以把你的 regex rewrite 调用进来
				auto resp = HttpResponse::newHttpResponse();
				resp->addHeader("Content-Type", contentType);
				resp->setBody(s);
				cb(resp);
				return;
			}
			else {
				auto resp = HttpResponse::newHttpResponse();
				if (!contentType.empty()) resp->addHeader("Content-Type", contentType);
				resp->setBody(body);
				cb(resp);
				return;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[proxy] exception: " << e.what() << "\n";
			auto r = HttpResponse::newHttpResponse();
			r->setStatusCode(HttpStatusCode::k500InternalServerError);
			r->setBody(std::string("internal error: ") + e.what());
			cb(r);
			return;
		}
		catch (...) {
			std::cerr << "[proxy] unknown exception\n";
			auto r = HttpResponse::newHttpResponse();
			r->setStatusCode(HttpStatusCode::k500InternalServerError);
			r->setBody("internal unknown error");
			cb(r);
			return;
		}
		}).detach();


}


