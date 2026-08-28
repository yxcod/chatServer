#pragma once

#include <cstdint>
#include <string>
#include <utility>

class MerchantReviewCommentModel
{
public:
    std::uint64_t getCommentId() const noexcept { return commentId_; }
    std::uint64_t getEntryId() const noexcept { return entryId_; }
    const std::string& getUserName() const noexcept { return userName_; }
    const std::string& getContent() const noexcept { return content_; }
    const std::string& getImageName() const noexcept { return imageName_; }
    std::uint8_t getStatus() const noexcept { return status_; }
    std::uint64_t getCreatedAt() const noexcept { return createdAt_; }
    std::uint64_t getUpdatedAt() const noexcept { return updatedAt_; }
    std::uint64_t getDeletedAt() const noexcept { return deletedAt_; }
    const std::string& getDisplayName() const noexcept { return displayName_; }
    const std::string& getAvatarName() const noexcept { return avatarName_; }

    void setCommentId(std::uint64_t value) noexcept { commentId_ = value; }
    void setEntryId(std::uint64_t value) noexcept { entryId_ = value; }
    void setUserName(std::string value) { userName_ = std::move(value); }
    void setContent(std::string value) { content_ = std::move(value); }
    void setImageName(std::string value) { imageName_ = std::move(value); }
    void setStatus(std::uint8_t value) noexcept { status_ = value; }
    void setCreatedAt(std::uint64_t value) noexcept { createdAt_ = value; }
    void setUpdatedAt(std::uint64_t value) noexcept { updatedAt_ = value; }
    void setDeletedAt(std::uint64_t value) noexcept { deletedAt_ = value; }
    void setDisplayName(std::string value) { displayName_ = std::move(value); }
    void setAvatarName(std::string value) { avatarName_ = std::move(value); }

private:
    std::uint64_t commentId_{0};
    std::uint64_t entryId_{0};
    std::string userName_;
    std::string content_;
    std::string imageName_;
    std::uint8_t status_{0};
    std::uint64_t createdAt_{0};
    std::uint64_t updatedAt_{0};
    std::uint64_t deletedAt_{0};
    std::string displayName_;
    std::string avatarName_;
};
