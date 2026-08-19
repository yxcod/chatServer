#pragma once
#include <string>
#include <cstdint>
#include <sstream>

// 对应表: groupConversations
class GroupConversationModel
{
public:
    GroupConversationModel() = default;

    GroupConversationModel(int id,
        uint64_t groupId,
        uint64_t updateTime,
        std::string lastSenderId,
        std::string lastMsg,
        std::string validList,
        uint8_t msgType)
        : id_(id),
        groupId_(groupId),
        updateTime_(updateTime),
        lastSenderId_(std::move(lastSenderId)),
        lastMsg_(std::move(lastMsg)),
        validList_(std::move(validList)),
        msgType_(msgType)
    {
    }

    GroupConversationModel(const GroupConversationModel&) = default;
    GroupConversationModel(GroupConversationModel&&) noexcept = default;
    GroupConversationModel& operator=(const GroupConversationModel&) = default;
    GroupConversationModel& operator=(GroupConversationModel&&) noexcept = default;
    ~GroupConversationModel() = default;

    // getters
    int getId() const noexcept { return id_; }
    uint64_t getGroupId() const noexcept { return groupId_; }
    uint64_t getUpdateTime() const noexcept { return updateTime_; }
    const std::string& getLastSenderId() const noexcept { return lastSenderId_; }
    const std::string& getLastMsg() const noexcept { return lastMsg_; }
    const std::string& getValidList() const noexcept { return validList_; }
    uint8_t getMsgType() const noexcept { return msgType_; }   // 消息类型：0-文本 1-图片 2-文件 3-语音

    // setters
    void setId(int v) { id_ = v; }
    void setGroupId(uint64_t v) { groupId_ = v; }
    void setUpdateTime(uint64_t v) { updateTime_ = v; }
    void setLastSenderId(const std::string& v) { lastSenderId_ = v; }
    void setLastMsg(const std::string& v) { lastMsg_ = v; }
    void setValidList(const std::string& v) { validList_ = v; }
    void setMsgType(uint8_t v) { msgType_ = v; }               // 设置消息类型

    std::string toString() const
    {
        std::ostringstream os;
        os << "GroupConversationModel{";
        os << "id=" << id_ << ", ";
        os << "groupId=" << groupId_ << ", ";
        os << "updateTime=" << updateTime_ << ", ";
        os << "lastSenderId=" << lastSenderId_ << ", ";
        os << "lastMsg=" << lastMsg_ << ", ";
        os << "validList=" << validList_ << ", ";
        os << "msgType=" << static_cast<int>(msgType_);
        os << "}";
        return os.str();
    }

private:
    int id_ = 0;                    // 主键ID
    uint64_t groupId_ = 0;          // 群组唯一标识
    uint64_t updateTime_ = 0;       // 最后更新时间戳
    std::string lastSenderId_;      // 最后发送者ID
    std::string lastMsg_;           // 最后一条消息内容
    std::string validList_;         // 有效成员/权限列表（JSON或逗号分隔）
    uint8_t msgType_ = 0;           // 消息类型：0-文本 1-图片 2-文件 3-语音
};