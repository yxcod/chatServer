#pragma once

#include <vector>
#include <string>
#include "Logger.h"
#include "GroupConversationModel.h"

class GroupConversationDao
{
public:
    GroupConversationDao();

    // 插入或更新（groupId 唯一：不存在插入，存在则更新最近消息信息）
    int insert(const GroupConversationModel& conv);

    // 根据 groupId 查询一条会话记录（不存在返回默认对象）
    GroupConversationModel getByGroupId(uint64_t groupId) const;

    // 兼容老的 upsert 接口
    bool upsert(const GroupConversationModel& conv);

    // 更新会话的最近消息信息（根据 groupId）
    bool updateLastMessage(uint64_t groupId,
        uint64_t updateTime,
        const std::string& lastSenderId,
        const std::string& lastMsg,
        const std::string& validList,
        uint8_t msgType);

    // 查询某用户所在的所有群会话（validList 模糊匹配）
    std::vector<GroupConversationModel> getConversationsByUser(const std::string& userId) const;
    // 根据 groupId 删除 groupConversations 对应行
    bool deleteByGroupId(uint64_t groupId);
};