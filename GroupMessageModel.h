#pragma once

#include <string>
#include <cstdint>
#include <sstream>

class GroupMessageModel {
public:
    GroupMessageModel() = default;

    GroupMessageModel(uint64_t msgId,
                      uint64_t groupId,
                      std::string senderId,
                      uint8_t msgType,
                      std::string msgContent,
                      std::string extendInfo,
                      uint64_t fileSize,
                      uint64_t sendTime,
                      uint8_t isDeleted,
                      uint8_t isRead)
        : msgId(msgId),
          groupId(groupId),
          senderId(std::move(senderId)),
          msgType(msgType),
          msgContent(std::move(msgContent)),
          extendInfo(std::move(extendInfo)),
          fileSize(fileSize),
          sendTime(sendTime),
          isDeleted(isDeleted),
          isRead(isRead) {}

    GroupMessageModel(const GroupMessageModel&) = default;
    GroupMessageModel(GroupMessageModel&&) noexcept = default;
    GroupMessageModel& operator=(const GroupMessageModel&) = default;
    GroupMessageModel& operator=(GroupMessageModel&&) noexcept = default;
    ~GroupMessageModel() = default;

    uint64_t getMsgId() const noexcept { return msgId; }
    uint64_t getGroupId() const noexcept { return groupId; }
    const std::string& getSenderId() const noexcept { return senderId; }
    uint8_t getMsgType() const noexcept { return msgType; }
    const std::string& getMsgContent() const noexcept { return msgContent; }
    const std::string& getExtendInfo() const noexcept { return extendInfo; }
    uint64_t getFileSize() const noexcept { return fileSize; }
    uint64_t getSendTime() const noexcept { return sendTime; }  // 时间戳
    uint8_t getIsDeleted() const noexcept { return isDeleted; }
    uint8_t getIsRead() const noexcept { return isRead; }

    void setMsgId(uint64_t v) { msgId = v; }
    void setGroupId(uint64_t v) { groupId = v; }
    void setSenderId(const std::string& v) { senderId = v; }
    void setMsgType(uint8_t v) { msgType = v; }
    void setMsgContent(const std::string& v) { msgContent = v; }
    void setExtendInfo(const std::string& v) { extendInfo = v; }
    void setFileSize(uint64_t v) { fileSize = v; }
    void setSendTime(uint64_t v) { sendTime = v; }              // 时间戳
    void setIsDeleted(uint8_t v) { isDeleted = v; }
    void setIsRead(uint8_t v) { isRead = v; }

    std::string toString() const
    {
        std::ostringstream os;
        os << "GroupMessageModel{";
        os << "msgId=" << msgId << ", ";
        os << "groupId=" << groupId << ", ";
        os << "senderId=" << senderId << ", ";
        os << "msgType=" << static_cast<int>(msgType) << ", ";
        os << "msgContent=" << msgContent << ", ";
        os << "extendInfo=" << extendInfo << ", ";
        os << "fileSize=" << fileSize << ", ";
        os << "sendTime=" << sendTime << ", ";
        os << "isDeleted=" << static_cast<int>(isDeleted) << ", ";
        os << "isRead=" << static_cast<int>(isRead);
        os << "}";
        return os.str();
    }

private:
    uint64_t msgId = 0;
    uint64_t groupId = 0;
    std::string senderId;
    uint8_t msgType = 0;
    std::string msgContent;
    std::string extendInfo{"{}"};
    uint64_t fileSize = 0;
    uint64_t sendTime = 0;   // 时间戳
    uint8_t isDeleted = 0;
    uint8_t isRead = 0;
};
