#pragma once

#include <string>
#include <cstdint>
#include <sstream>

// ConversationModel.h
// 对应数据库会话表的模型类（字段与给定SQL结构对应）
class ConversationModel {
public:
    // 默认构造
    ConversationModel() = default;

    // 全字段构造
    ConversationModel(std::string convId,
                      uint8_t convType,
                      std::string user1Id,
                      std::string user2Id,
                      std::string groupId,
                      std::string lastMsg,
                      std::string lastMsgId,
                      std::string lastSenderId,
                      int user1UnreadCount,
                      int user2UnreadCount,
                      uint64_t updateTime,
                      uint8_t user2isValid,
                      uint8_t user1isVaild)
        : convId(std::move(convId)),
          convType(convType),
          user1Id(std::move(user1Id)),
          user2Id(std::move(user2Id)),
          groupId(std::move(groupId)),
          lastMsg(std::move(lastMsg)),
          lastMsgId(std::move(lastMsgId)),
          lastSenderId(std::move(lastSenderId)),
          user1UnreadCount(user1UnreadCount),
          user2UnreadCount(user2UnreadCount),
          updateTime(updateTime),
          user2isValid(user2isValid),
          user1isVaild(user1isVaild) {}

    // 拷贝与移动默认
    ConversationModel(const ConversationModel&) = default;
    ConversationModel(ConversationModel&&) noexcept = default;
    ConversationModel& operator=(const ConversationModel&) = default;
    ConversationModel& operator=(ConversationModel&&) noexcept = default;

    ~ConversationModel() = default;

    // getters
    const std::string& getConvId() const noexcept { return convId; }
    uint8_t getConvType() const noexcept { return convType; }
    const std::string& getUser1Id() const noexcept { return user1Id; }
    const std::string& getUser2Id() const noexcept { return user2Id; }
    const std::string& getGroupId() const noexcept { return groupId; }
    const std::string& getLastMsg() const noexcept { return lastMsg; }
    const std::string& getLastMsgId() const noexcept { return lastMsgId; }
    const std::string& getLastSenderId() const noexcept { return lastSenderId; }
    int getUser1UnreadCount() const noexcept { return user1UnreadCount; }
    int getUser2UnreadCount() const noexcept { return user2UnreadCount; }
    uint64_t getUpdateTime() const noexcept { return updateTime; }
    uint8_t getUser2isValid() const noexcept { return user2isValid; }
    uint8_t getUser1isVaild() const noexcept { return user1isVaild; }

    // setters
    void setConvId(const std::string& v) { convId = v; }
    void setConvType(uint8_t v) { convType = v; }
    void setUser1Id(const std::string& v) { user1Id = v; }
    void setUser2Id(const std::string& v) { user2Id = v; }
    void setGroupId(const std::string& v) { groupId = v; }
    void setLastMsg(const std::string& v) { lastMsg = v; }
    void setLastMsgId(const std::string& v) { lastMsgId = v; }
    void setLastSenderId(const std::string& v) { lastSenderId = v; }
    void setUser1UnreadCount(int v) { user1UnreadCount = v; }
    void setUser2UnreadCount(int v) { user2UnreadCount = v; }
    void setUpdateTime(uint64_t v) { updateTime = v; }
    void setUser2isValid(uint8_t v) { user2isValid = v; }
    void setUser1isVaild(uint8_t v) { user1isVaild = v; }

    // 辅助：将模型转换为可读字符串（调试）
    std::string toString() const {
        std::ostringstream os;
        os << "ConversationModel{";
        os << "convId=" << convId << ", ";
        os << "convType=" << static_cast<int>(convType) << ", ";
        os << "user1Id=" << user1Id << ", ";
        os << "user2Id=" << user2Id << ", ";
        os << "groupId=" << groupId << ", ";
        os << "lastMsg='" << lastMsg << "', ";
        os << "lastMsgId=" << lastMsgId << ", ";
        os << "lastSenderId=" << lastSenderId << ", ";
        os << "user1UnreadCount=" << user1UnreadCount << ", ";
        os << "user2UnreadCount=" << user2UnreadCount << ", ";
        os << "updateTime=" << updateTime << ", ";
        os << "user2isValid=" << static_cast<int>(user2isValid) << ", ";
        os << "user1isVaild=" << static_cast<int>(user1isVaild);
        os << "}";
        return os.str();
    }

private:
    std::string convId;       // 会话唯一标识
    uint8_t convType = 1;     // 会话类型：1-单聊，2-群聊
    std::string user1Id;      // 单聊-用户A/群聊-群主ID
    std::string user2Id;      // 单聊-用户B，群聊为空
    std::string groupId;      // 群聊-关联群表ID，单聊为空
    std::string lastMsg;      // 会话最新一条消息内容
    std::string lastMsgId;    // 最新消息ID，关联消息表
    std::string lastSenderId; // 最新消息发送者ID
    int user1UnreadCount = 0; // 用户1未读消息数
    int user2UnreadCount = 0; // 用户2未读消息数
    uint64_t updateTime = 0;  // 会话最后更新时间
    uint8_t user2isValid = 1; // 用户2是否删除了会话：1-正常，0-已删除
    uint8_t user1isVaild = 1; // 用户1是否删除了会话：1-正常，0-已删除
};
