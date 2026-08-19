#include "GroupMessageDao.h"
#include "GroupMsgReadDao.h"
GroupMessageDao::GroupMessageDao() = default;

uint64_t GroupMessageDao::insertMessage(GroupMessageModel &msg)
{
    uint64_t newId = 0;
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "INSERT INTO groupMessage "
                "(groupId, senderId, msgType, msgContent, fileSize, sendTime, "
                "isDeleted, isRead) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"));

        pstmt->setUInt64(1, msg.getGroupId());
        pstmt->setString(2, msg.getSenderId());
        pstmt->setUInt(3, msg.getMsgType());
        pstmt->setString(4, msg.getMsgContent());
        pstmt->setUInt64(5, msg.getFileSize());
        pstmt->setUInt64(6, msg.getSendTime());     // 时间戳
        pstmt->setUInt(7, msg.getIsDeleted());
        pstmt->setUInt(8, msg.getIsRead());

        pstmt->executeUpdate();

        std::unique_ptr<sql::PreparedStatement> idStmt(
            con->prepareStatement("SELECT LAST_INSERT_ID() AS id"));
        std::unique_ptr<sql::ResultSet> res(idStmt->executeQuery());
        if (res->next())
        {
            newId = res->getUInt64("id");
            msg.setMsgId(newId);
        }
    }
    catch (const std::exception &e)
    {
        Logger::GetInstance().error(e.what());
    }
    return newId;
}

std::vector<GroupMessageModel> GroupMessageDao::getRecentMessages(uint64_t groupId,
                                                                  std::size_t limit) const
{
    std::vector<GroupMessageModel> msgs;
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::ostringstream sql;
        sql << "SELECT msgId, groupId, senderId, msgType, msgContent, "
            << "fileSize,sendTime, "
            << "isDeleted, isRead "
            << "FROM groupMessage "
            << "WHERE groupId = ? "
            << "ORDER BY sendTime ASC "
            << "LIMIT " << limit;

        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(sql.str()));
        pstmt->setUInt64(1, groupId);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            GroupMessageModel m;
            m.setMsgId(res->getUInt64("msgId"));
            m.setGroupId(res->getUInt64("groupId"));
            m.setSenderId(res->getString("senderId"));
            m.setMsgType(static_cast<uint8_t>(res->getUInt("msgType")));
            m.setMsgContent(res->getString("msgContent"));
            m.setFileSize(res->getUInt64("fileSize"));
            m.setSendTime(res->getUInt64("sendTime"));  // 时间戳
            m.setIsDeleted(static_cast<uint8_t>(res->getUInt("isDeleted")));
            m.setIsRead(static_cast<uint8_t>(res->getUInt("isRead")));
            msgs.push_back(std::move(m));
        }
    }
    catch (const std::exception &e)
    {
        Logger::GetInstance().error(e.what());
    }
    return msgs;
}

std::vector<GroupMessageModel> GroupMessageDao::getMessagesByTime(uint64_t groupId,
                                                                  const uint64_t& beginTime,
                                                                  const uint64_t& endTime) const
{
    std::vector<GroupMessageModel> msgs;
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT msgId, groupId, senderId, msgType, msgContent, "
                "fileSize, sendTime, isDeleted, isRead "
                "FROM groupMessage "
                "WHERE groupId = ? AND sendTime BETWEEN ? AND ? "
                "ORDER BY sendTime ASC"));

        pstmt->setUInt64(1, groupId);
        pstmt->setUInt64(2, beginTime);
        pstmt->setUInt64(3, endTime);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            GroupMessageModel m;
            m.setMsgId(res->getUInt64("msgId"));
            m.setGroupId(res->getUInt64("groupId"));
            m.setSenderId(res->getString("senderId"));      // 改为 string
            m.setMsgType(static_cast<uint8_t>(res->getUInt("msgType")));
            m.setMsgContent(res->getString("msgContent"));
            m.setFileSize(res->getUInt64("fileSize"));
            m.setSendTime(res->getUInt64("sendTime"));
            m.setIsDeleted(static_cast<uint8_t>(res->getUInt("isDeleted")));
            m.setIsRead(static_cast<uint8_t>(res->getUInt("isRead")));
            msgs.push_back(std::move(m));
        }
    }
    catch (const std::exception &e)
    {
        Logger::GetInstance().error(e.what());
    }
    return msgs;
}

std::vector<GroupMessageModel> GroupMessageDao::getUnreadMessagesByUserInGroup(const std::string& userId,
    uint64_t groupId) const
{
    std::vector<GroupMessageModel> result;
    try
    {
        auto con = Logger::GetInstance().createConnection();
        // 一条 SQL：在 groupMsgRead 过滤 userId/readTime=0，再关联 groupMessage 过滤 groupId
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT gm.msgId, gm.groupId, gm.senderId, gm.msgType, "
                "gm.msgContent, gm.fileSize, "
                "gm.sendTime AS sendTime, "
                "gm.isDeleted, gm.isRead "
                "FROM groupMsgRead gr "
                "JOIN groupMessage gm ON gr.msgId = gm.msgId "
                "WHERE gr.userId = ? "
                "AND gr.readTime = 0 "
                "AND gm.groupId = ? "
                "ORDER BY gm.sendTime ASC"));

        pstmt->setString(1, userId);
        pstmt->setUInt64(2, groupId);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            GroupMessageModel m;
            m.setMsgId(res->getUInt64("msgId"));
            m.setGroupId(res->getUInt64("groupId"));
            m.setSenderId(res->getString("senderId"));
            m.setMsgType(static_cast<uint8_t>(res->getUInt("msgType")));
            m.setMsgContent(res->getString("msgContent"));
            m.setFileSize(res->getUInt64("fileSize"));
            m.setSendTime(res->getUInt64("sendTime"));   // 时间戳
            m.setIsDeleted(static_cast<uint8_t>(res->getUInt("isDeleted")));
            m.setIsRead(static_cast<uint8_t>(res->getUInt("isRead")));
            result.push_back(std::move(m));
        }
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }

    return result;
}

// 已有的 getUnreadCountByUserAndGroup 可保持不变：
// 使用同样的关联条件，只是 SELECT COUNT(*)
int GroupMessageDao::getUnreadCountByUserAndGroup(const std::string& userId, uint64_t groupId) const
{
    int count = 0;
    try
    {
        auto con = Logger::GetInstance().createConnection();

        // 一条 SQL：在 groupMsgRead 过滤 userId/readTime=0，再关联 groupMessage 过滤 groupId 计数
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT COUNT(*) AS cnt "
                "FROM groupMsgRead gr "
                "JOIN groupMessage gm ON gr.msgId = gm.msgId "
                "WHERE gr.userId = ? "
                "AND gr.readTime = 0 "
                "AND gm.groupId = ?"));

        pstmt->setString(1, userId);
        pstmt->setUInt64(2, groupId);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (res->next())
        {
            count = static_cast<int>(res->getUInt("cnt"));
        }
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return count;
}

bool GroupMessageDao::deleteMessagesByGroupId(uint64_t groupId)
{
    try
    {
        auto con = Logger::GetInstance().createConnection();

        // 1. 查询该群所有 msgId
        std::unique_ptr<sql::PreparedStatement> selectStmt(
            con->prepareStatement(
                "SELECT msgId FROM groupMessage WHERE groupId = ?"));

        selectStmt->setUInt64(1, groupId);

        std::vector<uint64_t> msgIds;
        {
            std::unique_ptr<sql::ResultSet> res(selectStmt->executeQuery());
            while (res->next())
            {
                msgIds.push_back(res->getUInt64("msgId"));
            }
        }

        // 2. 逐条删除对应的已读记录
        GroupMsgReadDao readDao;
        for (uint64_t msgId : msgIds)
        {
            // 按你在 GroupMsgReadDao::deleteByMsgId 里的风格，
            // 没有记录时也视为成功，这里不做失败中断
            readDao.deleteByMsgId(msgId);
        }

        // 3. 删除 groupMessage 中该群的所有消息
        std::unique_ptr<sql::PreparedStatement> deleteStmt(
            con->prepareStatement(
                "DELETE FROM groupMessage WHERE groupId = ?"));

        deleteStmt->setUInt64(1, groupId);

        // 如果你希望“没有消息也算成功”，直接执行并返回 true
        deleteStmt->executeUpdate();
        return true;
        // 如果你想至少删到一行才算成功，就用：
        // return deleteStmt->executeUpdate() > 0;
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return false;
}