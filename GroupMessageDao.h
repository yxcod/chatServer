#pragma once

#include <vector>
#include <string>
#include "Logger.h"
#include "GroupMessageModel.h"

class GroupMessageDao
{
public:
    GroupMessageDao();

    // 插入一条群消息，返回自增 msgId（失败返回 0）
    uint64_t insertMessage(GroupMessageModel &msg);

    // 按群获取最近 N 条消息（按 sendTime DESC）
    std::vector<GroupMessageModel> getRecentMessages(uint64_t groupId,
                                                     std::size_t limit) const;

    // 按时间范围分页获取消息
    std::vector<GroupMessageModel> getMessagesByTime(uint64_t groupId,
                                                    const uint64_t& beginTime,
                                                    const uint64_t& endTime) const;
    // 修改：指定 userId 在指定 groupId 下的未读群消息
    std::vector<GroupMessageModel> getUnreadMessagesByUserInGroup(const std::string& userId,uint64_t groupId) const;
   // 指定用户在指定群里的未读消息条数
    int getUnreadCountByUserAndGroup(const std::string &userId, uint64_t groupId) const;
    // 根据 groupId 删除该群所有消息及其对应的已读记录
    bool deleteMessagesByGroupId(uint64_t groupId);
private:
};