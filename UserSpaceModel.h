#pragma once

#include <cstdint>
#include <string>
#include <utility>

class UserSpaceModel
{
public:
    const std::string& getOwnerUserName() const noexcept { return ownerUserName_; }
    const std::string& getCoverImageUrl() const noexcept { return coverImageUrl_; }
    std::uint64_t getCreatedAt() const noexcept { return createdAt_; }
    std::uint64_t getUpdatedAt() const noexcept { return updatedAt_; }

    void setOwnerUserName(std::string value) { ownerUserName_ = std::move(value); }
    void setCoverImageUrl(std::string value) { coverImageUrl_ = std::move(value); }
    void setCreatedAt(std::uint64_t value) noexcept { createdAt_ = value; }
    void setUpdatedAt(std::uint64_t value) noexcept { updatedAt_ = value; }

private:
    std::string ownerUserName_;
    std::string coverImageUrl_;
    std::uint64_t createdAt_{0};
    std::uint64_t updatedAt_{0};
};
