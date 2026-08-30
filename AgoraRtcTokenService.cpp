#include "AgoraRtcTokenService.h"

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <random>
#include <stdexcept>

#include <json/json.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <zlib.h>

namespace {
struct Config {
    std::string appId;
    std::string appCertificate;
    uint32_t tokenExpireSeconds = 3600;
};

void put16(std::string& out, uint16_t value) {
    out.push_back(static_cast<char>(value & 0xff));
    out.push_back(static_cast<char>((value >> 8) & 0xff));
}
void put32(std::string& out, uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        out.push_back(static_cast<char>((value >> shift) & 0xff));
}
std::string packString(const std::string& value) {
    if (value.size() > 0xffff) throw std::invalid_argument("Agora value too long");
    std::string out;
    put16(out, static_cast<uint16_t>(value.size()));
    return out + value;
}
std::string pack32(uint32_t value) {
    std::string out;
    put32(out, value);
    return out;
}
std::string hmacSha256(const std::string& key, const std::string& value) {
    unsigned int length = EVP_MAX_MD_SIZE;
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    HMAC(EVP_sha256(), key.data(), static_cast<int>(key.size()),
         reinterpret_cast<const unsigned char*>(value.data()), value.size(),
         digest.data(), &length);
    return {reinterpret_cast<const char*>(digest.data()), 32};
}
std::string deflateValue(const std::string& value) {
    uLongf length = compressBound(static_cast<uLong>(value.size()));
    std::string output(length, '\0');
    const auto result = compress2(reinterpret_cast<Bytef*>(output.data()), &length,
        reinterpret_cast<const Bytef*>(value.data()),
        static_cast<uLong>(value.size()), Z_DEFAULT_COMPRESSION);
    if (result != Z_OK) throw std::runtime_error("Agora token compression failed");
    output.resize(length);
    return output;
}
std::string base64(const std::string& value) {
    std::string output(4 * ((value.size() + 2) / 3), '\0');
    const int length = EVP_EncodeBlock(
        reinterpret_cast<unsigned char*>(output.data()),
        reinterpret_cast<const unsigned char*>(value.data()),
        static_cast<int>(value.size()));
    if (length <= 0) throw std::runtime_error("Agora token encoding failed");
    output.resize(static_cast<std::size_t>(length));
    return output;
}
bool isHex32(const std::string& value) {
    return value.size() == 32 &&
        value.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos;
}
Config loadConfig() {
    const auto path = std::filesystem::current_path() / "config" /
        "agora_rtc.local.json";
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Agora RTC config file is missing");
    Json::Value json;
    Json::CharReaderBuilder builder;
    std::string error;
    if (!Json::parseFromStream(builder, input, &json, &error))
        throw std::runtime_error("Agora RTC config JSON is invalid");
    Config config;
    config.appId = json["appId"].asString();
    config.appCertificate = json["appCertificate"].asString();
    if (json.isMember("tokenExpireSeconds"))
        config.tokenExpireSeconds = json["tokenExpireSeconds"].asUInt();
    if (!isHex32(config.appId) || !isHex32(config.appCertificate))
        throw std::runtime_error("Agora App ID or certificate is invalid");
    if (config.tokenExpireSeconds < 300 || config.tokenExpireSeconds > 86400)
        throw std::runtime_error("Agora token expiration must be 300-86400 seconds");
    return config;
}
uint32_t stableUid(const std::string& userId) {
    std::array<unsigned char, SHA256_DIGEST_LENGTH> digest{};
    SHA256(reinterpret_cast<const unsigned char*>(userId.data()),
           userId.size(), digest.data());
    uint32_t uid = (static_cast<uint32_t>(digest[0]) << 24) |
        (static_cast<uint32_t>(digest[1]) << 16) |
        (static_cast<uint32_t>(digest[2]) << 8) | digest[3];
    return uid == 0 ? 1 : uid;
}

// Token007 encoding follows Agora's official AccessToken2/RtcTokenBuilder2.
std::string buildToken(const Config& config, const std::string& channel,
                       uint32_t uid, uint32_t now, uint32_t salt) {
    const uint32_t privilegeExpire = config.tokenExpireSeconds;
    std::string service;
    put16(service, 1);  // RTC service type.
    put16(service, 4);
    for (uint16_t privilege = 1; privilege <= 4; ++privilege) {
        put16(service, privilege);
        put32(service, privilegeExpire);
    }
    service += packString(channel);
    service += packString(std::to_string(uid));
    std::string services;
    put16(services, 1);
    services += service;
    const auto signingInfo = packString(config.appId) + pack32(now) +
        pack32(config.tokenExpireSeconds) + pack32(salt) + services;
    auto signing = hmacSha256(pack32(now), config.appCertificate);
    signing = hmacSha256(pack32(salt), signing);
    const auto signature = hmacSha256(signing, signingInfo);
    return "007" + base64(deflateValue(packString(signature) + signingInfo));
}
}

AgoraRtcCredential AgoraRtcTokenService::issue(
    const std::string& userId, const std::string& channelName) const {
    if (channelName.empty() || channelName.size() >= 64 ||
        channelName.rfind("quanxin_", 0) != 0)
        throw std::invalid_argument("Invalid Agora channel name");
    const auto config = loadConfig();
    const auto now = static_cast<uint32_t>(std::chrono::duration_cast<
        std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    std::random_device random;
    const auto uid = stableUid(userId);
    return {config.appId, buildToken(config, channelName, uid, now, random()),
            uid, static_cast<uint64_t>(now) + config.tokenExpireSeconds};
}
