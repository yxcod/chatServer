#pragma once

#include <string>
#include <cstdint>
#include <sstream>

// GroupMsgReadModel.h
// 对应数据库 groupMsgRead 表的模型类
class GroupMsgReadModel {
public:
    // 默认构造
    GroupMsgReadModel() = default;

    // 全字段构造
    GroupMsgReadModel(uint64_t id,
                      uint64_t msgId,
                      std::string userId,
                      uint64_t readTime)
        : id(id),
          msgId(msgId),
          userId(std::move(userId)),
          readTime(readTime) {}

    // 拷贝与移动默认
    GroupMsgReadModel(const GroupMsgReadModel&) = default;
    GroupMsgReadModel(GroupMsgReadModel&&) noexcept = default;
    GroupMsgReadModel& operator=(const GroupMsgReadModel&) = default;
    GroupMsgReadModel& operator=(GroupMsgReadModel&&) noexcept = default;

    ~GroupMsgReadModel() = default;

    // getters
    uint64_t getId() const noexcept { return id; }
    uint64_t getMsgId() const noexcept { return msgId; }
    const std::string& getUserId() const noexcept { return userId; }
    uint64_t getReadTime() const noexcept { return readTime; }   // 时间戳

    // setters
    void setId(uint64_t v) { id = v; }
    void setMsgId(uint64_t v) { msgId = v; }
    void setUserId(const std::string& v) { userId = v; }
    void setReadTime(uint64_t v) { readTime = v; }

    // 调试用字符串输出
    std::string toString() const
    {
        std::ostringstream os;
        os << "GroupMsgReadModel{";
        os << "id=" << id << ", ";
        os << "msgId=" << msgId << ", ";
        os << "userId=" << userId << ", ";
        os << "readTime=" << readTime;
        os << "}";
        return os.str();
    }

private:
    uint64_t id = 0;        // 主键ID
    uint64_t msgId = 0;     // 消息ID
    std::string userId;     // 用户ID（字符串形式）
    uint64_t readTime = 0;   // 时间戳
};