#pragma once

#include <string>
#include <cstdint>
#include <sstream>

// GroupChatModel.h
// 对应数据库 groupChat 表的模型类
class GroupChatModel {
public:
    // 默认构造
    GroupChatModel() = default;

    // 全字段构造（含主键 id 和业务群号 groupId）
    GroupChatModel(uint64_t id,
        uint64_t groupId,
        std::string groupName,
        std::string groupAvatar,
        std::string creatorId,
        std::string description,
        uint32_t maxMembers,
        uint8_t isActive,
        uint64_t createdAt,
        uint64_t updatedAt)
        : id(id),
        groupId(groupId),
        groupName(std::move(groupName)),
        groupAvatar(std::move(groupAvatar)),
        creatorId(std::move(creatorId)),
        description(std::move(description)),
        maxMembers(maxMembers),
        isActive(isActive),
        createdAt(createdAt),
        updatedAt(updatedAt) {
    }

    // 拷贝与移动默认
    GroupChatModel(const GroupChatModel&) = default;
    GroupChatModel(GroupChatModel&&) noexcept = default;
    GroupChatModel& operator=(const GroupChatModel&) = default;
    GroupChatModel& operator=(GroupChatModel&&) noexcept = default;

    ~GroupChatModel() = default;

    // getters
    uint64_t getId() const noexcept { return id; }              // 主键ID
    uint64_t getGroupId() const noexcept { return groupId; }    // 业务群ID
    const std::string& getGroupName() const noexcept { return groupName; }
    const std::string& getGroupAvatar() const noexcept { return groupAvatar; }
    const std::string& getCreatorId() const noexcept { return creatorId; }
    const std::string& getDescription() const noexcept { return description; }
    uint32_t getMaxMembers() const noexcept { return maxMembers; }
    uint8_t getIsActive() const noexcept { return isActive; }
    uint64_t getCreatedAt() const noexcept { return createdAt; }
    uint64_t getUpdatedAt() const noexcept { return updatedAt; }

    // setters
    void setId(uint64_t v) { id = v; }
    void setGroupId(uint64_t v) { groupId = v; }
    void setGroupName(const std::string& v) { groupName = v; }
    void setGroupAvatar(const std::string& v) { groupAvatar = v; }
    void setCreatorId(const std::string& v) { creatorId = v; }
    void setDescription(const std::string& v) { description = v; }
    void setMaxMembers(uint32_t v) { maxMembers = v; }
    void setIsActive(uint8_t v) { isActive = v; }
    void setCreatedAt(uint64_t v) { createdAt = v; }
    void setUpdatedAt(uint64_t v) { updatedAt = v; }

    // 调试用字符串输出
    std::string toString() const
    {
        std::ostringstream os;
        os << "GroupChatModel{";
        os << "id=" << id << ", ";
        os << "groupId=" << groupId << ", ";
        os << "groupName=" << groupName << ", ";
        os << "groupAvatar=" << groupAvatar << ", ";
        os << "creatorId=" << creatorId << ", ";
        os << "description=" << description << ", ";
        os << "maxMembers=" << maxMembers << ", ";
        os << "isActive=" << static_cast<int>(isActive) << ", ";
        os << "createdAt=" << createdAt << ", ";
        os << "updatedAt=" << updatedAt;
        os << "}";
        return os.str();
    }

private:
    uint64_t id = 0;             // 主键ID（自增）
    uint64_t groupId = 0;        // 群聊业务ID（可自定/唯一）
    std::string groupName;       // 群聊名称
    std::string groupAvatar;     // 群头像
    std::string creatorId;       // 创建人用户ID（字符串形式）
    std::string description;     // 群聊描述（可为空）
    uint32_t maxMembers = 200;   // 最大成员数
    uint8_t isActive = 1;        // 是否有效：1-有效 0-解散
    uint64_t createdAt = 0;      // 创建时间（时间戳）
    uint64_t updatedAt = 0;      // 更新时间（时间戳）
};