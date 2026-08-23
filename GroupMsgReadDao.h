#pragma once

#include <vector>
#include <string>
#include "Logger.h"
#include "GroupMsgReadModel.h"

class GroupMsgReadDao
{
public:
    GroupMsgReadDao();
	// 插入一条已读记录，返回是否成功（成功后 model 会包含自增 id）
    bool insert(GroupMsgReadModel& model);

    // 标记记录已读（利用唯一键 msgId+userId，更新 readTime）
    bool markRead(const GroupMsgReadModel& model);

    // 将用户在指定群中、不晚于某条消息的所有未读记录一次性标记为已读。
    bool markGroupReadThrough(const std::string& userId,
                              uint64_t groupId,
                              uint64_t maxMsgId,
                              uint64_t readTime) const;

    // 查询某条消息所有已读用户
    std::vector<GroupMsgReadModel> getReadersByMsg(uint64_t msgId) const;

    // 查询某条消息的完整阅读状态，由服务层拆分已读和未读用户。
    std::vector<GroupMsgReadModel> getReadStatusesByMsg(uint64_t msgId) const;

    // 一次查询多条消息的阅读状态，避免打开群聊时逐条访问数据库。
    std::vector<GroupMsgReadModel> getReadStatusesByMessages(
        const std::vector<uint64_t>& msgIds) const;

    // 查询用户对某群消息的已读记录（可选）
    std::vector<GroupMsgReadModel> getUserReadRecords(const std::string& userId,
                                                      uint64_t msgIdBegin,
                                                      uint64_t msgIdEnd) const;
    // 批量插入多条已读记录（使用一条批量 INSERT SQL）
    bool insertBatch(const std::vector<GroupMsgReadModel>& models);

    // 根据 msgId 删除所有对应的已读记录
    bool deleteByMsgId(uint64_t msgId);
private:
};
