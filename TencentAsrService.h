#pragma once

#include <cstdint>
#include <functional>
#include <string>

struct TencentAsrResult {
    int providerCode{0};
    std::string providerMessage;
    std::string requestId;
    std::string transcript;
    std::uint32_t audioDurationMs{0};
};

class TencentAsrService {
public:
    using Completion = std::function<void(bool, TencentAsrResult)>;

    void transcribe(
        std::string audioData,
        const std::string& voiceFormat,
        Completion completion) const;

    static std::string configuredEngineType();
};
