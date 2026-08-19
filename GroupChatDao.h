#pragma once

#include <vector>
#include <string>
#include "Logger.h"
#include "GroupChatModel.h"

class GroupChatDao
{
public:
    GroupChatDao();

    // 新建群聊，返回自增 groupId（失败返回 0）
    uint64_t createGroup(const GroupChatModel &group);

    // 根据 groupId 获取群信息（不存在返回默认对象，调用方自行判断 groupId 是否为 0）
    GroupChatModel getGroupById(uint64_t groupId) const;

    // 查询用户创建的所有群
    std::vector<GroupChatModel> getGroupsByCreator(const std::string& creatorId) const;

    // 更新群基础信息（名称、头像、描述、最大成员数、是否有效）
    bool updateGroupInfo(uint64_t groupId, const GroupChatModel& group);
	//查询用户加入的所有群
    std::vector<GroupChatModel> getGroupsByUserId(const std::string& userId) const;
	// 判断群是否存在
    bool groupExists(uint64_t groupId) const;
    // 根据 groupId 删除 groupChat 对应行
    bool deleteGroupById(uint64_t groupId);

private:
};