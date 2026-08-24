#pragma once

#include <cstdint>
#include <string>
#include <utility>

class VoiceTranscriptionModel {
public:
    std::uint64_t getId() const noexcept { return id_; }
    const std::string& getAudioOwnerId() const noexcept { return audioOwnerId_; }
    const std::string& getAudioName() const noexcept { return audioName_; }
    const std::string& getAudioSha256() const noexcept { return audioSha256_; }
    const std::string& getEngineType() const noexcept { return engineType_; }
    const std::string& getTranscript() const noexcept { return transcript_; }
    std::uint32_t getAudioDurationMs() const noexcept { return audioDurationMs_; }
    const std::string& getProviderRequestId() const noexcept { return providerRequestId_; }
    std::uint64_t getCreatedAt() const noexcept { return createdAt_; }
    std::uint64_t getUpdatedAt() const noexcept { return updatedAt_; }

    void setId(std::uint64_t value) noexcept { id_ = value; }
    void setAudioOwnerId(std::string value) { audioOwnerId_ = std::move(value); }
    void setAudioName(std::string value) { audioName_ = std::move(value); }
    void setAudioSha256(std::string value) { audioSha256_ = std::move(value); }
    void setEngineType(std::string value) { engineType_ = std::move(value); }
    void setTranscript(std::string value) { transcript_ = std::move(value); }
    void setAudioDurationMs(std::uint32_t value) noexcept { audioDurationMs_ = value; }
    void setProviderRequestId(std::string value) { providerRequestId_ = std::move(value); }
    void setCreatedAt(std::uint64_t value) noexcept { createdAt_ = value; }
    void setUpdatedAt(std::uint64_t value) noexcept { updatedAt_ = value; }

private:
    std::uint64_t id_{0};
    std::string audioOwnerId_;
    std::string audioName_;
    std::string audioSha256_;
    std::string engineType_;
    std::string transcript_;
    std::uint32_t audioDurationMs_{0};
    std::string providerRequestId_;
    std::uint64_t createdAt_{0};
    std::uint64_t updatedAt_{0};
};
