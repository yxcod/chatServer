#pragma once

#include <cstdint>
#include <string>

class PrivateChatHistoryVisibilityModel
{
public:
    const std::string& getConversationId() const noexcept { return conversationId_; }
    const std::string& getUserId() const noexcept { return userId_; }
    const std::string& getPeerUserId() const noexcept { return peerUserId_; }
    uint64_t getDeletedThroughRecordId() const noexcept { return deletedThroughRecordId_; }
    uint64_t getUpdatedAt() const noexcept { return updatedAt_; }

    void setConversationId(const std::string& value) { conversationId_ = value; }
    void setUserId(const std::string& value) { userId_ = value; }
    void setPeerUserId(const std::string& value) { peerUserId_ = value; }
    void setDeletedThroughRecordId(uint64_t value) { deletedThroughRecordId_ = value; }
    void setUpdatedAt(uint64_t value) { updatedAt_ = value; }

private:
    std::string conversationId_;
    std::string userId_;
    std::string peerUserId_;
    uint64_t deletedThroughRecordId_ = 0;
    uint64_t updatedAt_ = 0;
};
