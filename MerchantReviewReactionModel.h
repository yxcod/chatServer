#pragma once

#include <cstdint>
#include <string>
#include <utility>

class MerchantReviewReactionModel
{
public:
    std::uint64_t getReactionId() const noexcept { return reactionId_; }
    std::uint64_t getEntryId() const noexcept { return entryId_; }
    const std::string& getUserName() const noexcept { return userName_; }
    std::uint8_t getReactionType() const noexcept { return reactionType_; }
    std::uint64_t getCreatedAt() const noexcept { return createdAt_; }
    std::uint64_t getUpdatedAt() const noexcept { return updatedAt_; }

    void setReactionId(std::uint64_t value) noexcept { reactionId_ = value; }
    void setEntryId(std::uint64_t value) noexcept { entryId_ = value; }
    void setUserName(std::string value) { userName_ = std::move(value); }
    void setReactionType(std::uint8_t value) noexcept { reactionType_ = value; }
    void setCreatedAt(std::uint64_t value) noexcept { createdAt_ = value; }
    void setUpdatedAt(std::uint64_t value) noexcept { updatedAt_ = value; }

private:
    std::uint64_t reactionId_{0};
    std::uint64_t entryId_{0};
    std::string userName_;
    std::uint8_t reactionType_{0};
    std::uint64_t createdAt_{0};
    std::uint64_t updatedAt_{0};
};
