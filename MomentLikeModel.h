#pragma once

#include <cstdint>
#include <string>
#include <utility>

class MomentLikeModel
{
public:
    std::uint64_t getLikeId() const noexcept { return likeId_; }
    std::uint64_t getMomentId() const noexcept { return momentId_; }
    const std::string& getUserName() const noexcept { return userName_; }
    std::uint64_t getCreatedAt() const noexcept { return createdAt_; }

    void setLikeId(std::uint64_t value) noexcept { likeId_ = value; }
    void setMomentId(std::uint64_t value) noexcept { momentId_ = value; }
    void setUserName(std::string value) { userName_ = std::move(value); }
    void setCreatedAt(std::uint64_t value) noexcept { createdAt_ = value; }

private:
    std::uint64_t likeId_{0};
    std::uint64_t momentId_{0};
    std::string userName_;
    std::uint64_t createdAt_{0};
};
