#pragma once

#include <cstdint>
#include <string>
#include <utility>

class UserBlockModel
{
public:
    std::uint64_t getId() const noexcept { return id_; }
    const std::string& getBlockerUserName() const noexcept { return blockerUserName_; }
    const std::string& getBlockedUserName() const noexcept { return blockedUserName_; }
    std::uint64_t getCreatedAt() const noexcept { return createdAt_; }

    void setId(std::uint64_t value) noexcept { id_ = value; }
    void setBlockerUserName(std::string value) { blockerUserName_ = std::move(value); }
    void setBlockedUserName(std::string value) { blockedUserName_ = std::move(value); }
    void setCreatedAt(std::uint64_t value) noexcept { createdAt_ = value; }

private:
    std::uint64_t id_{0};
    std::string blockerUserName_;
    std::string blockedUserName_;
    std::uint64_t createdAt_{0};
};
