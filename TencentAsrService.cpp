#include "TencentAsrService.h"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <json/json.h>
#include <map>
#include <memory>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <sstream>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>

namespace {
struct TencentAsrConfig {
    std::string appId;
    std::string secretId;
    std::string secretKey;
    std::string engineType{"16k_zh"};
};

std::vector<std::filesystem::path> configCandidates()
{
    std::vector<std::filesystem::path> candidates{
        std::filesystem::current_path() / "config" / "tencent_asr.local.json"
    };
#ifdef _WIN32
    std::vector<wchar_t> executablePath(32768, L'\0');
    const DWORD length = GetModuleFileNameW(
        nullptr, executablePath.data(), static_cast<DWORD>(executablePath.size()));
    if (length > 0 && length < executablePath.size()) {
        const auto besideExecutable =
            std::filesystem::path(executablePath.data()).parent_path() /
            "config" / "tencent_asr.local.json";
        if (besideExecutable != candidates.front()) {
            candidates.push_back(besideExecutable);
        }
    }
#endif
    return candidates;
}

TencentAsrConfig loadConfig()
{
    std::ifstream input;
    for (const auto& candidate : configCandidates()) {
        input.open(candidate, std::ios::binary);
        if (input) {
            break;
        }
        input.clear();
    }
    if (!input) {
        throw std::runtime_error(
            "Tencent ASR config not found in the working or executable directory");
    }
    Json::CharReaderBuilder builder;
    Json::Value root;
    std::string errors;
    if (!Json::parseFromStream(builder, input, &root, &errors)) {
        throw std::runtime_error("Tencent ASR config is not valid JSON");
    }

    TencentAsrConfig config;
    const Json::Value& appId = root.isMember("appId")
        ? root["appId"] : root["AppID"];
    if (appId.isString()) {
        config.appId = appId.asString();
    } else if (appId.isUInt64()) {
        config.appId = std::to_string(appId.asUInt64());
    }
    if (root.isMember("secretId")) {
        config.secretId = root["secretId"].asString();
    } else if (root.isMember("SecretId")) {
        config.secretId = root["SecretId"].asString();
    } else {
        config.secretId = root["SecretID"].asString();
    }
    config.secretKey = root.isMember("secretKey")
        ? root["secretKey"].asString() : root["SecretKey"].asString();
    if (root["engineType"].isString() && !root["engineType"].asString().empty()) {
        config.engineType = root["engineType"].asString();
    }

    const bool appIdIsNumeric = !config.appId.empty() &&
        std::all_of(config.appId.begin(), config.appId.end(), [](unsigned char value) {
            return std::isdigit(value) != 0;
        });
    if (!appIdIsNumeric || config.secretId.empty() || config.secretKey.empty() ||
        config.engineType.empty()) {
        throw std::runtime_error("Tencent ASR config is missing required fields");
    }
    return config;
}

std::string percentEncode(const std::string& value)
{
    std::ostringstream encoded;
    encoded << std::uppercase << std::hex;
    for (const unsigned char character : value) {
        if (std::isalnum(character) || character == '-' || character == '_' ||
            character == '.' || character == '~') {
            encoded << static_cast<char>(character);
        } else {
            encoded << '%' << std::setw(2) << std::setfill('0')
                << static_cast<int>(character);
        }
    }
    return encoded.str();
}

std::string makeQuery(const std::map<std::string, std::string>& parameters)
{
    std::ostringstream query;
    bool first = true;
    for (const auto& parameter : parameters) {
        if (!first) query << '&';
        first = false;
        query << percentEncode(parameter.first) << '=' << percentEncode(parameter.second);
    }
    return query.str();
}

std::string sign(const std::string& text, const std::string& secretKey)
{
    unsigned int digestLength = EVP_MAX_MD_SIZE;
    unsigned char digest[EVP_MAX_MD_SIZE]{};
    if (HMAC(EVP_sha1(), secretKey.data(), static_cast<int>(secretKey.size()),
        reinterpret_cast<const unsigned char*>(text.data()), text.size(),
        digest, &digestLength) == nullptr) {
        throw std::runtime_error("Failed to sign Tencent ASR request");
    }

    std::string base64(((digestLength + 2) / 3) * 4, '\0');
    const int encodedLength = EVP_EncodeBlock(
        reinterpret_cast<unsigned char*>(base64.data()), digest, digestLength);
    if (encodedLength <= 0) {
        throw std::runtime_error("Failed to encode Tencent ASR signature");
    }
    base64.resize(static_cast<std::size_t>(encodedLength));
    return base64;
}

TencentAsrResult parseResponse(const drogon::HttpResponsePtr& response)
{
    TencentAsrResult result;
    if (!response) {
        result.providerCode = -1;
        result.providerMessage = "Tencent ASR returned an empty response";
        return result;
    }
    result.httpStatus = static_cast<int>(response->getStatusCode());
    std::string body(response->body());
    if (body.size() >= 3 &&
        static_cast<unsigned char>(body[0]) == 0xef &&
        static_cast<unsigned char>(body[1]) == 0xbb &&
        static_cast<unsigned char>(body[2]) == 0xbf) {
        body.erase(0, 3);
    }
    const auto previewLength = std::min<std::size_t>(body.size(), 300);
    result.providerBodyPreview.assign(body.data(), previewLength);
    std::replace_if(result.providerBodyPreview.begin(),
        result.providerBodyPreview.end(), [](unsigned char character) {
            return character < 0x20 || character == 0x7f;
        }, ' ');

    Json::CharReaderBuilder builder;
    Json::Value json;
    std::string errors;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    if (body.empty() || !reader->parse(
        body.data(), body.data() + body.size(), &json, &errors)) {
        result.providerCode = -1;
        result.providerMessage = "Tencent ASR returned invalid JSON (HTTP " +
            std::to_string(result.httpStatus) + ")";
        return result;
    }
    result.providerCode = json["code"].asInt();
    result.providerMessage = json["message"].asString();
    result.requestId = json["request_id"].asString();
    result.audioDurationMs = json["audio_duration"].asUInt();
    const auto& flashResult = json["flash_result"];
    if (flashResult.isArray() && !flashResult.empty()) {
        result.transcript = flashResult[0]["text"].asString();
    }
    return result;
}
}

void TencentAsrService::transcribe(
    std::string audioData,
    const std::string& voiceFormat,
    Completion completion) const
{
    TencentAsrConfig config;
    try {
        config = loadConfig();
    } catch (const std::exception& error) {
        TencentAsrResult result;
        result.providerCode = -1;
        result.providerMessage = error.what();
        completion(false, std::move(result));
        return;
    }

    const std::map<std::string, std::string> parameters{
        {"convert_num_mode", "1"},
        {"engine_type", config.engineType},
        {"filter_dirty", "0"},
        {"filter_modal", "0"},
        {"filter_punc", "0"},
        {"first_channel_only", "1"},
        {"secretid", config.secretId},
        {"speaker_diarization", "0"},
        {"timestamp", std::to_string(static_cast<long long>(std::time(nullptr)))},
        {"voice_format", voiceFormat},
        {"word_info", "0"}
    };
    const std::string query = makeQuery(parameters);
    const std::string basePath = "/asr/flash/v1/" + config.appId;
    const std::string path = basePath + "?" + query;

    std::string authorization;
    try {
        authorization = sign("POSTasr.cloud.tencent.com" + path, config.secretKey);
    } catch (const std::exception& error) {
        TencentAsrResult result;
        result.providerCode = -1;
        result.providerMessage = error.what();
        completion(false, std::move(result));
        return;
    }

    try {
        auto request = drogon::HttpRequest::newHttpRequest();
        request->setMethod(drogon::Post);
        // Drogon encodes setPath() by default. Encoding the '?' turns the
        // signed query into part of the path and Tencent responds with an
        // HTML gateway/404 body instead of its documented JSON payload.
        request->setPathEncode(false);
        request->setPath(path);
        request->setContentTypeCode(drogon::CT_APPLICATION_OCTET_STREAM);
        request->addHeader("Host", "asr.cloud.tencent.com");
        request->addHeader("Authorization", authorization);
        request->addHeader("Accept", "application/json");
        request->addHeader("Accept-Encoding", "identity");
        // Drogon writes Content-Length automatically from the binary body.
        // Adding it manually produces two Content-Length headers, which the
        // Tencent nginx gateway rejects with HTTP 400 before ASR validation.
        request->setBody(std::move(audioData));

        auto client = drogon::HttpClient::newHttpClient("https://asr.cloud.tencent.com");
        client->sendRequest(request,
            [client, completion = std::move(completion)](
                drogon::ReqResult requestResult,
                const drogon::HttpResponsePtr& response) mutable {
                if (requestResult != drogon::ReqResult::Ok) {
                    TencentAsrResult result;
                    result.providerCode = -1;
                    result.providerMessage = "Tencent ASR request failed";
                    completion(false, std::move(result));
                    return;
                }
                auto result = parseResponse(response);
                completion(result.providerCode == 0, std::move(result));
            },
            20.0);
    } catch (const std::exception& error) {
        TencentAsrResult result;
        result.providerCode = -1;
        result.providerMessage = error.what();
        completion(false, std::move(result));
    }
}

std::string TencentAsrService::configuredEngineType()
{
    return loadConfig().engineType;
}
