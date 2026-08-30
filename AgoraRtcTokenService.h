#pragma once
#include <cstdint>
#include <string>

struct AgoraRtcCredential {
    std::string appId;
    std::string token;
    uint32_t uid = 0;
    uint64_t expiresAt = 0;
};

class AgoraRtcTokenService {
public:
    AgoraRtcCredential issue(const std::string& userId,
                             const std::string& channelName) const;
};
