#include "TencentAsrService.h"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <json/json.h>
#include <memory>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
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

std::string trimWhitespace(std::string value)
{
    const auto first = std::find_if_not(value.begin(), value.end(),
        [](unsigned char character) { return std::isspace(character) != 0; });
    const auto last = std::find_if_not(value.rbegin(), value.rend(),
        [](unsigned char character) { return std::isspace(character) != 0; }).base();
    if (first >= last) return {};
    return std::string(first, last);
}

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

    // Values copied from a browser or edited in Notepad can contain invisible
    // leading/trailing whitespace. Tencent includes SecretId and SecretKey in
    // signature validation, so even one extra character makes the signature
    // invalid and its gateway may answer with an HTML 404 response.
    config.appId = trimWhitespace(std::move(config.appId));
    config.secretId = trimWhitespace(std::move(config.secretId));
    config.secretKey = trimWhitespace(std::move(config.secretKey));
    config.engineType = trimWhitespace(std::move(config.engineType));

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

std::string hexEncode(const unsigned char* data, std::size_t length)
{
    std::ostringstream encoded;
    encoded << std::hex << std::setfill('0');
    for (std::size_t index = 0; index < length; ++index) {
        encoded << std::setw(2) << static_cast<unsigned int>(data[index]);
    }
    return encoded.str();
}

std::string sha256Hex(const std::string& value)
{
    unsigned char digest[SHA256_DIGEST_LENGTH]{};
    SHA256(reinterpret_cast<const unsigned char*>(value.data()),
        value.size(), digest);
    return hexEncode(digest, SHA256_DIGEST_LENGTH);
}

std::string hmacSha256(const std::string& key, const std::string& value)
{
    unsigned int digestLength = EVP_MAX_MD_SIZE;
    unsigned char digest[EVP_MAX_MD_SIZE]{};
    if (HMAC(EVP_sha256(), key.data(), static_cast<int>(key.size()),
        reinterpret_cast<const unsigned char*>(value.data()), value.size(),
        digest, &digestLength) == nullptr) {
        throw std::runtime_error("Failed to create Tencent ASR signature");
    }
    return std::string(reinterpret_cast<const char*>(digest), digestLength);
}

std::string base64Encode(const std::string& value)
{
    std::string base64(((value.size() + 2) / 3) * 4, '\0');
    const int encodedLength = EVP_EncodeBlock(
        reinterpret_cast<unsigned char*>(base64.data()),
        reinterpret_cast<const unsigned char*>(value.data()),
        static_cast<int>(value.size()));
    if (encodedLength <= 0) {
        throw std::runtime_error("Failed to encode Tencent ASR audio");
    }
    base64.resize(static_cast<std::size_t>(encodedLength));
    return base64;
}

std::string utcDate(std::time_t timestamp)
{
    std::tm utc{};
#ifdef _WIN32
    if (gmtime_s(&utc, &timestamp) != 0) {
        throw std::runtime_error("Failed to create Tencent ASR timestamp");
    }
#else
    if (gmtime_r(&timestamp, &utc) == nullptr) {
        throw std::runtime_error("Failed to create Tencent ASR timestamp");
    }
#endif
    std::ostringstream date;
    date << std::put_time(&utc, "%Y-%m-%d");
    return date.str();
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
        if (result.httpStatus == 404) {
            result.providerMessage =
                "Tencent ASR gateway returned HTTP 404; verify the ASR "
                "credentials and service activation";
        } else {
            result.providerMessage = "Tencent ASR returned invalid JSON (HTTP " +
                std::to_string(result.httpStatus) + ")";
        }
        return result;
    }
    if (json["Response"].isObject()) {
        const auto& providerResponse = json["Response"];
        result.requestId = providerResponse["RequestId"].asString();
        if (providerResponse["Error"].isObject()) {
            result.providerCode = -1;
            const auto& error = providerResponse["Error"];
            result.providerMessage = error["Code"].asString();
            if (!error["Message"].asString().empty()) {
                result.providerMessage += ": " + error["Message"].asString();
            }
            return result;
        }
        result.providerCode = 0;
        result.transcript = providerResponse["Result"].asString();
        result.audioDurationMs = providerResponse["AudioDuration"].asUInt();
        return result;
    }

    result.providerCode = json["code"].asInt();
    result.providerMessage = json["message"].asString();
    result.requestId = json["request_id"].asString();
    result.audioDurationMs = json["audio_duration"].asUInt();
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

    std::string requestBody;
    std::string authorization;
    std::string timestampText;
    try {
        const std::string encodedAudio = base64Encode(audioData);
        constexpr std::size_t kMaximumEncodedAudioSize = 3ULL * 1024 * 1024;
        if (encodedAudio.size() > kMaximumEncodedAudioSize) {
            throw std::runtime_error(
                "Tencent sentence recognition audio exceeds the 3MB limit");
        }

        Json::Value payload;
        payload["EngSerViceType"] = config.engineType;
        payload["SourceType"] = 1;
        payload["VoiceFormat"] = voiceFormat;
        payload["Data"] = encodedAudio;
        payload["DataLen"] = static_cast<Json::UInt64>(audioData.size());
        payload["SubServiceType"] = 2;
        payload["ProjectId"] = 0;
        payload["WordInfo"] = 0;
        payload["FilterDirty"] = 0;
        payload["FilterModal"] = 0;
        payload["FilterPunc"] = 0;
        payload["ConvertNumMode"] = 1;
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        requestBody = Json::writeString(writer, payload);

        const auto timestamp = std::time(nullptr);
        timestampText = std::to_string(static_cast<long long>(timestamp));
        const std::string date = utcDate(timestamp);
        const std::string canonicalHeaders =
            "content-type:application/json; charset=utf-8\n"
            "host:asr.tencentcloudapi.com\n";
        const std::string signedHeaders = "content-type;host";
        const std::string canonicalRequest =
            "POST\n/\n\n" + canonicalHeaders + "\n" + signedHeaders +
            "\n" + sha256Hex(requestBody);
        const std::string credentialScope = date + "/asr/tc3_request";
        const std::string stringToSign =
            "TC3-HMAC-SHA256\n" + timestampText + "\n" +
            credentialScope + "\n" + sha256Hex(canonicalRequest);
        const std::string secretDate = hmacSha256("TC3" + config.secretKey, date);
        const std::string secretService = hmacSha256(secretDate, "asr");
        const std::string secretSigning =
            hmacSha256(secretService, "tc3_request");
        const std::string signatureBytes =
            hmacSha256(secretSigning, stringToSign);
        const std::string signature = hexEncode(
            reinterpret_cast<const unsigned char*>(signatureBytes.data()),
            signatureBytes.size());
        authorization =
            "TC3-HMAC-SHA256 Credential=" + config.secretId + "/" +
            credentialScope + ", SignedHeaders=" + signedHeaders +
            ", Signature=" + signature;
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
        request->setPath("/");
        request->setContentTypeString("application/json; charset=utf-8");
        request->addHeader("Host", "asr.tencentcloudapi.com");
        request->addHeader("Authorization", authorization);
        request->addHeader("Accept", "application/json");
        request->addHeader("Accept-Encoding", "identity");
        request->addHeader("X-TC-Action", "SentenceRecognition");
        request->addHeader("X-TC-Timestamp", timestampText);
        request->addHeader("X-TC-Version", "2019-06-14");
        request->addHeader("X-TC-Region", "ap-shanghai");
        request->setBody(std::move(requestBody));

        auto client = drogon::HttpClient::newHttpClient(
            "https://asr.tencentcloudapi.com");
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
