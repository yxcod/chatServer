#pragma once
#include <drogon/HttpController.h>
#include <iostream>
using namespace drogon;

class ProxyController : public drogon::HttpController<ProxyController> {
public:
	METHOD_LIST_BEGIN
	// POST /fetch
	ADD_METHOD_TO(ProxyController::fetch, "/fetch", Post);
	// GET /proxy?url=...
	ADD_METHOD_TO(ProxyController::proxyGet, "/proxy", Get);
	METHOD_LIST_END
	void fetch(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
	void proxyGet(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
	
};
