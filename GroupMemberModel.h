#pragma once

#include <string>
#include <cstdint>
#include <sstream>

// GroupMemberModel.h
// 对应数据库 groupMember 表的模型类
class GroupMemberModel {
public:
    // 默认构造
    GroupMemberModel() = default;

    // 全字段构造
    GroupMemberModel(uint64_t id,
        uint64_t groupId,
        std::string userId,
        uint8_t role,
        uint64_t joinTime,
        uint64_t quitTime,
        uint8_t isQuit,
        std::string groupNickName,
        uint8_t isMuted = 0,
        std::string mutedBy = {},
        uint64_t mutedAt = 0)
        : id(id),
        groupId(groupId),
        userId(std::move(userId)),
        role(role),
        joinTime(joinTime),
        quitTime(quitTime),
        isQuit(isQuit),
        groupNickName(std::move(groupNickName)),
        isMuted(isMuted),
        mutedBy(std::move(mutedBy)),
        mutedAt(mutedAt) {
    }

    // 拷贝与移动默认
    GroupMemberModel(const GroupMemberModel&) = default;
    GroupMemberModel(GroupMemberModel&&) noexcept = default;
    GroupMemberModel& operator=(const GroupMemberModel&) = default;
    GroupMemberModel& operator=(GroupMemberModel&&) noexcept = default;

    ~GroupMemberModel() = default;

    // getters
    uint64_t getId() const noexcept { return id; }
    uint64_t getGroupId() const noexcept { return groupId; }
    const std::string& getUserId() const noexcept { return userId; }
    uint8_t getRole() const noexcept { return role; }
    uint64_t getJoinTime() const noexcept { return joinTime; }  // 时间戳
    uint64_t getQuitTime() const noexcept { return quitTime; }  // 时间戳
    uint8_t getIsQuit() const noexcept { return isQuit; }
    const std::string& getGroupNickName() const noexcept { return groupNickName; }
    uint8_t getIsMuted() const noexcept { return isMuted; }
    const std::string& getMutedBy() const noexcept { return mutedBy; }
    uint64_t getMutedAt() const noexcept { return mutedAt; }

    // setters
    void setId(uint64_t v) { id = v; }
    void setGroupId(uint64_t v) { groupId = v; }
    void setUserId(const std::string& v) { userId = v; }
    void setRole(uint8_t v) { role = v; }
    void setJoinTime(uint64_t v) { joinTime = v; }
    void setQuitTime(uint64_t v) { quitTime = v; }
    void setIsQuit(uint8_t v) { isQuit = v; }
    void setGroupNickName(const std::string& v) { groupNickName = v; }
    void setIsMuted(uint8_t v) { isMuted = v; }
    void setMutedBy(const std::string& v) { mutedBy = v; }
    void setMutedAt(uint64_t v) { mutedAt = v; }

    // 调试用字符串输出
    std::string toString() const
    {
        std::ostringstream os;
        os << "GroupMemberModel{";
        os << "id=" << id << ", ";
        os << "groupId=" << groupId << ", ";
        os << "userId=" << userId << ", ";
        os << "role=" << static_cast<int>(role) << ", ";
        os << "joinTime=" << joinTime << ", ";
        os << "quitTime=" << quitTime << ", ";
        os << "isQuit=" << static_cast<int>(isQuit) << ", ";
        os << "groupNickName=" << groupNickName << ", ";
        os << "isMuted=" << static_cast<int>(isMuted) << ", ";
        os << "mutedBy=" << mutedBy << ", ";
        os << "mutedAt=" << mutedAt;
        os << "}";
        return os.str();
    }

private:
    uint64_t id = 0;           // 主键ID
    uint64_t groupId = 0;      // 群聊ID
    std::string userId;        // 用户ID（字符串形式）
    uint8_t role = 0;          // 成员角色：0-普通成员 1-管理员 2-群主
    uint64_t joinTime = 0;     // 加入时间，时间戳
    uint64_t quitTime = 0;     // 退出时间时间戳，0 表示未退出
    uint8_t isQuit = 0;        // 是否退出：0-未退出 1-已退出
    std::string groupNickName; // 在群里的昵称
    uint8_t isMuted = 0;       // 是否被禁言：0-否 1-是
    std::string mutedBy;       // 执行禁言的群成员 ID
    uint64_t mutedAt = 0;      // 禁言时间戳
};
