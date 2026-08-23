#pragma once
#include <cstdint>
#include <string>
#include <utility>

class UserLocationModel {
public:
    const std::string& getUserName() const noexcept { return userName_; }
    double getLatitude() const noexcept { return latitude_; }
    double getLongitude() const noexcept { return longitude_; }
    double getAccuracy() const noexcept { return accuracy_; }
    std::uint64_t getUpdatedAt() const noexcept { return updatedAt_; }
    void setUserName(std::string v) { userName_ = std::move(v); }
    void setLatitude(double v) noexcept { latitude_ = v; }
    void setLongitude(double v) noexcept { longitude_ = v; }
    void setAccuracy(double v) noexcept { accuracy_ = v; }
    void setUpdatedAt(std::uint64_t v) noexcept { updatedAt_ = v; }
private:
    std::string userName_; double latitude_{0}; double longitude_{0};
    double accuracy_{0}; std::uint64_t updatedAt_{0};
};
