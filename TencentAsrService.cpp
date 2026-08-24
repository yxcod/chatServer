#include "TencentAsrService.h"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <json/json.h>
#include <map>
#include <memory>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <sstream>
#include <stdexcept>

#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>

namespace {
struct TencentAsrConfig {
    std::string appId;
    std::string secretId;
    std::string secretKey;
    std::string engineType{"16k_zh"};
};

TencentAsrConfig loadConfig()
{
    const std::string path = "./config/tencent_asr.local.json";
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Tencent ASR config not found at " + path);
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
    const auto json = response->getJsonObject();
    if (!json) {
        result.providerCode = -1;
        result.providerMessage = "Tencent ASR returned invalid JSON";
        return result;
    }
    result.providerCode = (*json)["code"].asInt();
    result.providerMessage = (*json)["message"].asString();
    result.requestId = (*json)["request_id"].asString();
    result.audioDurationMs = (*json)["audio_duration"].asUInt();
    const auto& flashResult = (*json)["flash_result"];
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
    const std::string path = "/asr/flash/v1/" + config.appId + "?" + query;

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
        request->setPath(path);
        request->setContentTypeCode(drogon::CT_APPLICATION_OCTET_STREAM);
        request->addHeader("Host", "asr.cloud.tencent.com");
        request->addHeader("Authorization", authorization);
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
