#pragma once

#include <vector>
#include <string>
#include <optional>
#include "Logger.h"
#include "GroupMemberModel.h"

class GroupMemberDao
{
public:
    GroupMemberDao();

    // 加群（已经存在则更新 isQuit）
    bool addMember(GroupMemberModel &member);

    // 根据 groupId 查询所有成员
    std::vector<GroupMemberModel> getMembersByGroup(uint64_t groupId) const;

    // 根据 userId 查询加入的所有群
    std::vector<GroupMemberModel> getGroupsByUser(const std::string& userId) const;

    // 标记成员退出群
    bool markQuit(uint64_t groupId, const std::string& userId, uint64_t quitTime);
    // 判断某 user 是否在指定群中（可根据需要排除已退出成员）
    bool isUserInGroup(uint64_t groupId, const std::string& userId) const;
    // 返回有效群成员的角色：0-普通成员，1-管理员，2-群主。
    std::optional<uint8_t> getActiveMemberRole(
        uint64_t groupId,
        const std::string& userId) const;
    // 更新用户在群内的昵称（groupNickName）
    bool updateGroupNickName(uint64_t groupId,
        const std::string& userId,
        const std::string& groupNickName);

    // 更新用户在群内的身份（role：0-普通成员 1-管理员 2-群主）
    bool updateGroupRole(uint64_t groupId,
        const std::string& userId,
        uint8_t role);
    // 根据 groupId 删除该群的所有成员记录
    bool deleteByGroupId(uint64_t groupId);

private:
};
