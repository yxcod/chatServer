#pragma once

#include <cstdint>
#include <string>
#include <utility>

class MomentNotificationModel
{
public:
    std::uint64_t getNotificationId() const noexcept { return notificationId_; }
    const std::string& getRecipientUserName() const noexcept { return recipientUserName_; }
    const std::string& getActorUserName() const noexcept { return actorUserName_; }
    std::uint64_t getMomentId() const noexcept { return momentId_; }
    std::uint8_t getInteractionType() const noexcept { return interactionType_; }
    const std::string& getCommentContent() const noexcept { return commentContent_; }
    bool isRead() const noexcept { return isRead_; }
    std::uint64_t getCreatedAt() const noexcept { return createdAt_; }
    std::uint64_t getReadAt() const noexcept { return readAt_; }
    const std::string& getActorNickName() const noexcept { return actorNickName_; }
    const std::string& getActorAvatar() const noexcept { return actorAvatar_; }

    void setNotificationId(std::uint64_t value) noexcept { notificationId_ = value; }
    void setRecipientUserName(std::string value) { recipientUserName_ = std::move(value); }
    void setActorUserName(std::string value) { actorUserName_ = std::move(value); }
    void setMomentId(std::uint64_t value) noexcept { momentId_ = value; }
    void setInteractionType(std::uint8_t value) noexcept { interactionType_ = value; }
    void setCommentContent(std::string value) { commentContent_ = std::move(value); }
    void setRead(bool value) noexcept { isRead_ = value; }
    void setCreatedAt(std::uint64_t value) noexcept { createdAt_ = value; }
    void setReadAt(std::uint64_t value) noexcept { readAt_ = value; }
    void setActorNickName(std::string value) { actorNickName_ = std::move(value); }
    void setActorAvatar(std::string value) { actorAvatar_ = std::move(value); }

private:
    std::uint64_t notificationId_{0};
    std::string recipientUserName_;
    std::string actorUserName_;
    std::uint64_t momentId_{0};
    std::uint8_t interactionType_{0};
    std::string commentContent_;
    bool isRead_{false};
    std::uint64_t createdAt_{0};
    std::uint64_t readAt_{0};
    std::string actorNickName_;
    std::string actorAvatar_;
};
