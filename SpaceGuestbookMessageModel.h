#pragma once

#include <cstdint>
#include <string>
#include <utility>

class SpaceGuestbookMessageModel
{
public:
    std::uint64_t getMessageId() const noexcept { return messageId_; }
    const std::string& getOwnerUserName() const noexcept { return ownerUserName_; }
    const std::string& getAuthorUserName() const noexcept { return authorUserName_; }
    const std::string& getContent() const noexcept { return content_; }
    std::uint8_t getStatus() const noexcept { return status_; }
    std::uint64_t getCreatedAt() const noexcept { return createdAt_; }
    std::uint64_t getUpdatedAt() const noexcept { return updatedAt_; }
    std::uint64_t getDeletedAt() const noexcept { return deletedAt_; }
    const std::string& getAuthorNickName() const noexcept { return authorNickName_; }
    const std::string& getAuthorAvatar() const noexcept { return authorAvatar_; }

    void setMessageId(std::uint64_t value) noexcept { messageId_ = value; }
    void setOwnerUserName(std::string value) { ownerUserName_ = std::move(value); }
    void setAuthorUserName(std::string value) { authorUserName_ = std::move(value); }
    void setContent(std::string value) { content_ = std::move(value); }
    void setStatus(std::uint8_t value) noexcept { status_ = value; }
    void setCreatedAt(std::uint64_t value) noexcept { createdAt_ = value; }
    void setUpdatedAt(std::uint64_t value) noexcept { updatedAt_ = value; }
    void setDeletedAt(std::uint64_t value) noexcept { deletedAt_ = value; }
    void setAuthorNickName(std::string value) { authorNickName_ = std::move(value); }
    void setAuthorAvatar(std::string value) { authorAvatar_ = std::move(value); }

private:
    std::uint64_t messageId_{0};
    std::string ownerUserName_;
    std::string authorUserName_;
    std::string content_;
    std::uint8_t status_{0};
    std::uint64_t createdAt_{0};
    std::uint64_t updatedAt_{0};
    std::uint64_t deletedAt_{0};
    std::string authorNickName_;
    std::string authorAvatar_;
};
