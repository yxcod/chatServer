#pragma once

#include <cstdint>
#include <string>
#include <utility>

class PushDeviceRegistrationModel
{
public:
    std::uint64_t getId() const noexcept { return id_; }
    const std::string& getUserName() const noexcept { return userName_; }
    const std::string& getRegistrationId() const noexcept { return registrationId_; }
    const std::string& getPlatform() const noexcept { return platform_; }
    const std::string& getDeviceId() const noexcept { return deviceId_; }
    bool isEnabled() const noexcept { return enabled_; }
    bool isAppForeground() const noexcept { return appForeground_; }
    bool isBannerEnabled() const noexcept { return bannerEnabled_; }
    bool isSoundEnabled() const noexcept { return soundEnabled_; }
    bool isVibrationEnabled() const noexcept { return vibrationEnabled_; }
    std::uint64_t getCreatedAt() const noexcept { return createdAt_; }
    std::uint64_t getUpdatedAt() const noexcept { return updatedAt_; }

    void setId(std::uint64_t value) noexcept { id_ = value; }
    void setUserName(std::string value) { userName_ = std::move(value); }
    void setRegistrationId(std::string value) { registrationId_ = std::move(value); }
    void setPlatform(std::string value) { platform_ = std::move(value); }
    void setDeviceId(std::string value) { deviceId_ = std::move(value); }
    void setEnabled(bool value) noexcept { enabled_ = value; }
    void setAppForeground(bool value) noexcept { appForeground_ = value; }
    void setBannerEnabled(bool value) noexcept { bannerEnabled_ = value; }
    void setSoundEnabled(bool value) noexcept { soundEnabled_ = value; }
    void setVibrationEnabled(bool value) noexcept { vibrationEnabled_ = value; }
    void setCreatedAt(std::uint64_t value) noexcept { createdAt_ = value; }
    void setUpdatedAt(std::uint64_t value) noexcept { updatedAt_ = value; }

private:
    std::uint64_t id_{0};
    std::string userName_;
    std::string registrationId_;
    std::string platform_{"android"};
    std::string deviceId_;
    bool enabled_{true};
    bool appForeground_{false};
    bool bannerEnabled_{true};
    bool soundEnabled_{true};
    bool vibrationEnabled_{true};
    std::uint64_t createdAt_{0};
    std::uint64_t updatedAt_{0};
};
