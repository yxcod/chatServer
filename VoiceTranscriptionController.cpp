#include "VoiceTranscriptionController.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <json/json.h>
#include <openssl/sha.h>
#include <optional>
#include <sstream>

#include "JwtTokenUtil.h"
#include "Logger.h"
#include "TencentAsrService.h"
#include "VoiceTranscriptionDao.h"

namespace {
const std::filesystem::path kAudioRoot = "./audioData";

drogon::HttpResponsePtr jsonResponse(
    int code,
    const std::string& message,
    drogon::HttpStatusCode status = drogon::k200OK)
{
    Json::Value value;
    value["code"] = code;
    value["message"] = message;
    auto response = drogon::HttpResponse::newHttpJsonResponse(value);
    response->setStatusCode(status);
    return response;
}

bool isSafePathSegment(const std::string& value)
{
    if (value.empty() || value == "." || value == ".." || value.size() > 191) {
        return false;
    }
    return std::none_of(value.begin(), value.end(), [](unsigned char character) {
        return character < 0x20 || character == 0x7f || character == '/' ||
            character == '\\' || character == ':';
    });
}

std::optional<std::string> authenticatedUser(
    const drogon::HttpRequestPtr& request)
{
    static JwtTokenUtil tokenUtil(
        "c9bb708f526d420ea88d83cd316d662921646869efaf425eb150ab99d20f48bc");
    const auto token = tokenUtil.extractBearerToken(request);
    if (!token || !tokenUtil.verifyToken(*token)) return std::nullopt;
    const auto payload = tokenUtil.parsePayload(*token);
    const auto user = payload.find("userId");
    if (user == payload.end() || user->second.empty()) return std::nullopt;
    return user->second;
}

std::optional<std::string> readAudio(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) return std::nullopt;
    const auto size = input.tellg();
    if (size <= 0 || size > static_cast<std::streamoff>(10ULL * 1024 * 1024)) {
        return std::nullopt;
    }
    std::string data(static_cast<std::size_t>(size), '\0');
    input.seekg(0);
    if (!input.read(data.data(), size)) return std::nullopt;
    return data;
}

std::string sha256(const std::string& data)
{
    unsigned char digest[SHA256_DIGEST_LENGTH]{};
    SHA256(reinterpret_cast<const unsigned char*>(data.data()), data.size(), digest);
    std::ostringstream result;
    result << std::hex << std::setfill('0');
    for (const unsigned char byte : digest) {
        result << std::setw(2) << static_cast<int>(byte);
    }
    return result.str();
}

std::string voiceFormat(const std::string& audioName)
{
    auto extension = std::filesystem::path(audioName).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    if (extension == ".m4a" || extension == ".mp4") return "m4a";
    if (extension == ".aac") return "aac";
    return {};
}

Json::Value successResponse(
    const VoiceTranscriptionModel& item,
    bool cached)
{
    Json::Value response;
    response["code"] = 100;
    response["text"] = item.getTranscript();
    response["audioDurationMs"] = item.getAudioDurationMs();
    response["cached"] = cached;
    return response;
}
}

void VoiceTranscriptionController::transcribe(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    const auto json = request->getJsonObject();
    if (!json) {
        callback(jsonResponse(400, "Invalid JSON", drogon::k400BadRequest));
        return;
    }
    const std::string userName = (*json)["userName"].asString();
    const std::string ownerId = (*json)["ownerId"].asString();
    const std::string audioName = (*json)["audioName"].asString();
    const auto requester = authenticatedUser(request);
    if (!requester || *requester != userName) {
        callback(jsonResponse(401, "Unauthorized", drogon::k401Unauthorized));
        return;
    }
    if (!isSafePathSegment(ownerId) || !isSafePathSegment(audioName)) {
        callback(jsonResponse(400, "Invalid audio path", drogon::k400BadRequest));
        return;
    }
    const std::string format = voiceFormat(audioName);
    if (format.empty()) {
        callback(jsonResponse(415, "Unsupported audio format", drogon::k415UnsupportedMediaType));
        return;
    }

    const auto data = readAudio(kAudioRoot / ownerId / audioName);
    if (!data) {
        callback(jsonResponse(404, "Audio not found or invalid", drogon::k404NotFound));
        return;
    }

    std::string engineType;
    try {
        engineType = TencentAsrService::configuredEngineType();
    } catch (const std::exception& error) {
        Logger::GetInstance().error(
            std::string("Tencent ASR configuration error: ") + error.what());
        callback(jsonResponse(503, "Voice transcription is not configured",
            drogon::k503ServiceUnavailable));
        return;
    }

    const std::string audioSha256 = sha256(*data);
    try {
        const auto cached = VoiceTranscriptionDao().find(
            ownerId, audioName, audioSha256, engineType);
        if (cached) {
            callback(drogon::HttpResponse::newHttpJsonResponse(
                successResponse(*cached, true)));
            return;
        }
    } catch (const std::exception& error) {
        Logger::GetInstance().warning(
            std::string("Voice transcription cache lookup failed: ") + error.what());
    }

    TencentAsrService service;
    service.transcribe(*data, format,
        [callback = std::move(callback), ownerId, audioName, audioSha256,
            engineType](bool succeeded, TencentAsrResult result) mutable {
            if (!succeeded) {
                Logger::GetInstance().warning(
                    "Tencent ASR request failed: code=" +
                    std::to_string(result.providerCode) +
                    ", requestId=" + result.requestId);
                Json::Value response;
                response["code"] = 502;
                response["message"] = result.providerMessage.empty()
                    ? "Voice transcription failed" : result.providerMessage;
                response["providerCode"] = result.providerCode;
                response["requestId"] = result.requestId;
                auto httpResponse = drogon::HttpResponse::newHttpJsonResponse(response);
                httpResponse->setStatusCode(drogon::k502BadGateway);
                callback(httpResponse);
                return;
            }

            VoiceTranscriptionModel item;
            item.setAudioOwnerId(ownerId);
            item.setAudioName(audioName);
            item.setAudioSha256(audioSha256);
            item.setEngineType(engineType);
            item.setTranscript(result.transcript);
            item.setAudioDurationMs(result.audioDurationMs);
            item.setProviderRequestId(result.requestId);
            const auto now = Logger::GetInstance().getcurrentTime();
            item.setCreatedAt(now);
            item.setUpdatedAt(now);
            try {
                if (!VoiceTranscriptionDao().upsert(item)) {
                    Logger::GetInstance().warning("Voice transcription cache write returned false");
                }
            } catch (const std::exception& error) {
                Logger::GetInstance().warning(
                    std::string("Voice transcription cache write failed: ") + error.what());
            }
            callback(drogon::HttpResponse::newHttpJsonResponse(
                successResponse(item, false)));
        });
}
