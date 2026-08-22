#include "ChatDao.h"
#include "Logger.h"
#include <stdexcept>

std::vector<ConversationModel> ChatDao::getUserAllConversation(const std::string& userId) const
{
    std::vector<ConversationModel> conversations;

    std::string query =
        "SELECT * "
        "FROM conversations "
        "WHERE (user1Id = ? AND user1isVaild = 1) OR (user2Id = ? AND user2isValid = 1) "
        "ORDER BY updateTime DESC";

    auto con = Logger::GetInstance().createConnection();

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(query));
        pstmt->setString(1, userId);
        pstmt->setString(2, userId);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            ConversationModel cm;
            cm.setConvId(res->getString("convId"));
            cm.setConvType(static_cast<uint8_t>(res->getUInt("convType")));
            cm.setUser1Id(res->getString("user1Id"));
            cm.setUser2Id(res->getString("user2Id"));
            cm.setGroupId(res->getString("groupId"));
            cm.setLastMsg(res->getString("lastMsg"));
            cm.setLastMsgId(res->getString("lastMsgId"));
            cm.setLastSenderId(res->getString("lastSenderId"));
            cm.setUser1UnreadCount(res->getInt("user1UnreadCount"));
            cm.setUser2UnreadCount(res->getInt("user2UnreadCount"));
            cm.setUpdateTime(res->getUInt64("updateTime"));
            cm.setUser2isValid(static_cast<uint8_t>(res->getUInt("user2isValid")));
            cm.setUser1isVaild(static_cast<uint8_t>(res->getUInt("user1isVaild")));

            conversations.push_back(std::move(cm));
        }
    }
    catch (...)
    {
        return conversations;
    }

    return conversations;
}

int ChatDao::deleteConversationByConvId(const std::string& convId) const
{
    std::string sql = "DELETE FROM conversations WHERE convId = ?";

    auto con = Logger::GetInstance().createConnection();

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(sql));
        pstmt->setString(1, convId);
        int affected = pstmt->executeUpdate();
        return affected;
    }
    catch (...)
    {
        return 0;
    }
}

int ChatDao::updateConversation(const ConversationModel& conversation) const
{
    std::string sql =
        "INSERT INTO conversations (convId, convType, user1Id, user2Id, groupId, lastMsg, lastMsgId, lastSenderId, "
        "user1UnreadCount, user2UnreadCount, updateTime, user2isValid, user1isVaild) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
        "ON DUPLICATE KEY UPDATE "
        "convType = VALUES(convType), user1Id = VALUES(user1Id), user2Id = VALUES(user2Id), groupId = VALUES(groupId), "
        "lastMsg = VALUES(lastMsg), lastMsgId = VALUES(lastMsgId), lastSenderId = VALUES(lastSenderId), "
        "user1UnreadCount = VALUES(user1UnreadCount), user2UnreadCount = VALUES(user2UnreadCount), "
        "updateTime = VALUES(updateTime), user2isValid = VALUES(user2isValid), user1isVaild = VALUES(user1isVaild)";

    auto con = Logger::GetInstance().createConnection();

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(sql));
        pstmt->setString(1, conversation.getConvId());
        pstmt->setUInt(2, static_cast<unsigned int>(conversation.getConvType()));
        pstmt->setString(3, conversation.getUser1Id());
        pstmt->setString(4, conversation.getUser2Id());
        pstmt->setString(5, conversation.getGroupId());
        pstmt->setString(6, conversation.getLastMsg());
        pstmt->setString(7, conversation.getLastMsgId());
        pstmt->setString(8, conversation.getLastSenderId());
        pstmt->setInt(9, conversation.getUser1UnreadCount());
        pstmt->setInt(10, conversation.getUser2UnreadCount());
        pstmt->setUInt64(11, conversation.getUpdateTime());
        pstmt->setUInt(12, conversation.getUser2isValid());
        pstmt->setUInt(13, conversation.getUser1isVaild());
        int affected = pstmt->executeUpdate();
        return affected;
    }
    catch (...)
    {
        return 0;
    }
}

bool ChatDao::getConversationByConvId(
    const std::string& convId,
    ConversationModel& conversation) const
{
    auto con = Logger::GetInstance().createConnection();
    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
            "SELECT * FROM conversations WHERE convId = ? LIMIT 1"));
        pstmt->setString(1, convId);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res->next()) return false;

        conversation.setConvId(res->getString("convId"));
        conversation.setConvType(static_cast<uint8_t>(res->getUInt("convType")));
        conversation.setUser1Id(res->getString("user1Id"));
        conversation.setUser2Id(res->getString("user2Id"));
        conversation.setGroupId(res->getString("groupId"));
        conversation.setLastMsg(res->getString("lastMsg"));
        conversation.setLastMsgId(res->getString("lastMsgId"));
        conversation.setLastSenderId(res->getString("lastSenderId"));
        conversation.setUser1UnreadCount(res->getInt("user1UnreadCount"));
        conversation.setUser2UnreadCount(res->getInt("user2UnreadCount"));
        conversation.setUpdateTime(res->getUInt64("updateTime"));
        conversation.setUser2isValid(static_cast<uint8_t>(res->getUInt("user2isValid")));
        conversation.setUser1isVaild(static_cast<uint8_t>(res->getUInt("user1isVaild")));
        return true;
    }
    catch (...) { return false; }
}

bool ChatDao::insertChatRecordAndUpdateConversation(
    const ChatRecord& record,
    const ConversationModel& conversation) const
{
    auto con = Logger::GetInstance().createConnection();
    try
    {
        con->setAutoCommit(false);

        std::unique_ptr<sql::PreparedStatement> recordStmt(con->prepareStatement(
            "INSERT INTO chatrecord (id, msgId, sendUserId, receiveType, receiveId, msgType, "
            "msgContent, msgStatus, sendTime, readTime, extendInfo, sessionId) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
        recordStmt->setUInt64(1, record.getId());
        recordStmt->setUInt64(2, record.getMsgId());
        recordStmt->setString(3, record.getSendUserId());
        recordStmt->setUInt(4, record.getReceiveTypeAsUInt8());
        recordStmt->setString(5, record.getReceiveId());
        recordStmt->setUInt(6, record.getMsgTypeAsUInt8());
        recordStmt->setString(7, record.getMsgContent());
        recordStmt->setUInt(8, record.getMsgStatusAsUInt8());
        recordStmt->setUInt64(9, record.getSendTime());
        recordStmt->setUInt64(10, record.getReadTime());
        recordStmt->setString(11, record.getExtendInfo());
        recordStmt->setString(12, record.getSessionId());
        if (recordStmt->executeUpdate() <= 0)
        {
            throw std::runtime_error("failed to insert private chat record");
        }

        std::unique_ptr<sql::PreparedStatement> conversationStmt(con->prepareStatement(
            "INSERT INTO conversations (convId, convType, user1Id, user2Id, groupId, lastMsg, "
            "lastMsgId, lastSenderId, user1UnreadCount, user2UnreadCount, updateTime, "
            "user2isValid, user1isVaild) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
            "ON DUPLICATE KEY UPDATE convType = VALUES(convType), user1Id = VALUES(user1Id), "
            "user2Id = VALUES(user2Id), groupId = VALUES(groupId), lastMsg = VALUES(lastMsg), "
            "lastMsgId = VALUES(lastMsgId), lastSenderId = VALUES(lastSenderId), "
            "user1UnreadCount = user1UnreadCount + IF(VALUES(lastSenderId) = user2Id, 1, 0), "
            "user2UnreadCount = user2UnreadCount + IF(VALUES(lastSenderId) = user1Id, 1, 0), "
            "updateTime = VALUES(updateTime), user2isValid = VALUES(user2isValid), "
            "user1isVaild = VALUES(user1isVaild)"));
        conversationStmt->setString(1, conversation.getConvId());
        conversationStmt->setUInt(2, conversation.getConvType());
        conversationStmt->setString(3, conversation.getUser1Id());
        conversationStmt->setString(4, conversation.getUser2Id());
        conversationStmt->setString(5, conversation.getGroupId());
        conversationStmt->setString(6, conversation.getLastMsg());
        conversationStmt->setString(7, conversation.getLastMsgId());
        conversationStmt->setString(8, conversation.getLastSenderId());
        conversationStmt->setInt(9, conversation.getUser1UnreadCount());
        conversationStmt->setInt(10, conversation.getUser2UnreadCount());
        conversationStmt->setUInt64(11, conversation.getUpdateTime());
        conversationStmt->setUInt(12, conversation.getUser2isValid());
        conversationStmt->setUInt(13, conversation.getUser1isVaild());
        conversationStmt->executeUpdate();

        con->commit();
        con->setAutoCommit(true);
        return true;
    }
    catch (const std::exception& e)
    {
        try
        {
            con->rollback();
            con->setAutoCommit(true);
        }
        catch (...) {}
        Logger::GetInstance().error(e.what());
        return false;
    }
}

std::vector<ChatRecord> ChatDao::getChatRecordsByReceiveId(const std::string& receiveId, size_t limit) const
{
    std::vector<ChatRecord> records;

    std::string sql =
        "SELECT id, msgId, sendUserId, receiveType, receiveId, msgType, msgContent, "
        "msgStatus, sendTime, readTime, extendInfo, sessionId "
        "FROM chatrecord WHERE receiveId = ? ORDER BY sendTime DESC LIMIT ?";

    auto con = Logger::GetInstance().createConnection();

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(sql));
        pstmt->setString(1, receiveId);
        pstmt->setUInt(2, static_cast<unsigned int>(limit));
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        while (res->next())
        {
            ChatRecord rec;
            rec.setId(res->getUInt64("id"));
            rec.setMsgId(res->getUInt64("msgId"));
            rec.setSendUserId(res->getString("sendUserId"));
            rec.setReceiveType(static_cast<ChatRecord::ReceiveType>(res->getUInt("receiveType")));
            rec.setReceiveId(res->getString("receiveId"));
            rec.setMsgType(static_cast<ChatRecord::MsgType>(res->getUInt("msgType")));
            rec.setMsgContent(res->getString("msgContent"));
            rec.setMsgStatus(static_cast<ChatRecord::MsgStatus>(res->getUInt("msgStatus")));

            uint64_t sendTs = res->getUInt64("sendTime");
            rec.setSendTime(sendTs);

            uint64_t readTs = 0;
            try
            {
                readTs = res->getUInt64("readTime");
            }
            catch (...)
            {
                readTs = 0;
            }
            if (readTs != 0)
            {
                rec.setReadTime(readTs);
            }

            rec.setExtendInfo(static_cast<std::string>(res->getString("extendInfo")));
            rec.setSessionId(res->getString("sessionId"));

            records.push_back(std::move(rec));
        }
    }
    catch (...)
    {
        return records;
    }

    return records;
}

std::vector<ChatRecord> ChatDao::getUnreadMessage(const std::string& userName)
{
    std::vector<ChatRecord> records;

    std::string sql = "SELECT * FROM chatrecord WHERE msgStatus = ? and receiveId = ?";

    auto con = Logger::GetInstance().createConnection();

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(sql));
        pstmt->setInt(1, 1);
        pstmt->setString(2, userName);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        while (res->next())
        {
            ChatRecord rec;
            rec.setId(res->getUInt64("id"));
            rec.setMsgId(res->getUInt64("msgId"));
            rec.setSendUserId(res->getString("sendUserId"));
            rec.setReceiveType(static_cast<ChatRecord::ReceiveType>(res->getUInt("receiveType")));
            rec.setReceiveId(res->getString("receiveId"));
            rec.setMsgType(static_cast<ChatRecord::MsgType>(res->getUInt("msgType")));
            rec.setMsgContent(res->getString("msgContent"));
            rec.setMsgStatus(static_cast<ChatRecord::MsgStatus>(res->getUInt("msgStatus")));

            uint64_t sendTs = res->getUInt64("sendTime");
            rec.setSendTime(sendTs);

            uint64_t readTs = 0;
            try
            {
                readTs = res->getUInt64("readTime");
            }
            catch (...)
            {
                readTs = 0;
            }
            if (readTs != 0)
            {
                rec.setReadTime(readTs);
            }

            rec.setExtendInfo(static_cast<std::string>(res->getString("extendInfo")));
            rec.setSessionId(res->getString("sessionId"));

            records.push_back(std::move(rec));
        }
    }
    catch (...)
    {
        return records;
    }

    return records;
}

std::vector<ChatRecord> ChatDao::getChatRecordsBySessionId(const std::string& sessionId) const
{
    std::vector<ChatRecord> records;

    std::string sql =
        "SELECT id, msgId, sendUserId, receiveType, receiveId, msgType, msgContent, "
        "msgStatus, sendTime, readTime, extendInfo, sessionId "
        "FROM chatrecord WHERE sessionId = ? ORDER BY sendTime ASC";

    auto con = Logger::GetInstance().createConnection();

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(sql));
        pstmt->setString(1, sessionId);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        while (res->next())
        {
            ChatRecord rec;
            rec.setId(res->getUInt64("id"));
            rec.setMsgId(res->getUInt64("msgId"));
            rec.setSendUserId(res->getString("sendUserId"));
            rec.setReceiveType(static_cast<ChatRecord::ReceiveType>(res->getUInt("receiveType")));
            rec.setReceiveId(res->getString("receiveId"));
            rec.setMsgType(static_cast<ChatRecord::MsgType>(res->getUInt("msgType")));
            rec.setMsgContent(res->getString("msgContent"));
            rec.setMsgStatus(static_cast<ChatRecord::MsgStatus>(res->getUInt("msgStatus")));

            uint64_t sendTs = res->getUInt64("sendTime");
            rec.setSendTime(sendTs);

            uint64_t readTs = 0;
            try
            {
                readTs = res->getUInt64("readTime");
            }
            catch (...)
            {
                readTs = 0;
            }
            if (readTs != 0)
            {
                rec.setReadTime(readTs);
            }

            rec.setExtendInfo(static_cast<std::string>(res->getString("extendInfo")));
            rec.setSessionId(res->getString("sessionId"));

            records.push_back(std::move(rec));
        }
    }
    catch (...)
    {
        return records;
    }

    return records;
}

int ChatDao::insertChatRecord(const ChatRecord& record) const
{
    std::string sql =
        "INSERT INTO chatrecord (id, msgId, sendUserId, receiveType, receiveId, msgType, "
        "msgContent, msgStatus, sendTime, readTime, extendInfo, sessionId) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    auto con = Logger::GetInstance().createConnection();

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(sql));
        pstmt->setUInt64(1, record.getId());
        pstmt->setUInt64(2, record.getMsgId());
        pstmt->setString(3, record.getSendUserId());
        pstmt->setUInt(4, static_cast<unsigned int>(record.getReceiveTypeAsUInt8()));
        pstmt->setString(5, record.getReceiveId());
        pstmt->setUInt(6, static_cast<unsigned int>(record.getMsgTypeAsUInt8()));
        pstmt->setString(7, record.getMsgContent());
        pstmt->setUInt(8, static_cast<unsigned int>(record.getMsgStatusAsUInt8()));

        uint64_t sendT = record.getSendTime() != 0
            ? record.getSendTime()
            : Logger::GetInstance().getcurrentTime();
        pstmt->setUInt64(9, sendT);

        if (record.getReadTime() != 0)
        {
            uint64_t readT = record.getReadTime();
            pstmt->setUInt64(10, readT);
        }
        else
        {
            pstmt->setUInt64(10, 0);
        }

        pstmt->setString(11, record.getExtendInfo());
        pstmt->setString(12, record.getSessionId());

        int affected = pstmt->executeUpdate();
        return affected;
    }
    catch (...)
    {
        return 0;
    }
}

int ChatDao::deleteChatRecordById(uint64_t id) const
{
    std::string sql = "DELETE FROM chatrecord WHERE id = ?";

    auto con = Logger::GetInstance().createConnection();

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(sql));
        pstmt->setUInt64(1, id);
        int affected = pstmt->executeUpdate();
        return affected;
    }
    catch (...)
    {
        return 0;
    }
}

int ChatDao::deleteChatRecordsBetweenUsers(const std::string& sessionId) const
{
    std::string sql = "DELETE FROM chatrecord WHERE sessionId = ?";

    auto con = Logger::GetInstance().createConnection();

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(sql));
        pstmt->setString(1, sessionId);
        int affected = pstmt->executeUpdate();
        return affected;
    }
    catch (...)
    {
        return 0;
    }
}

int ChatDao::updateMsgStatusByMsgId(uint64_t msgId, uint8_t msgStatus) const
{
    std::string sql = "UPDATE chatrecord SET msgStatus = ? WHERE msgId = ?";

    auto con = Logger::GetInstance().createConnection();

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(sql));
        pstmt->setUInt(1, static_cast<unsigned int>(msgStatus));
        pstmt->setUInt64(2, msgId);
        int affected = pstmt->executeUpdate();
        return affected;
    }
    catch (...)
    {
        return 0;
    }
}

int ChatDao::resetUnreadCountForUser(const std::string& convId, const std::string& userName) const
{
    // 根据传入的 userName 是否匹配 user1Id / user2Id 来决定清零哪个未读计数
    // 这里在一条 SQL 中处理两种情况：
    // 如果 userName == user1Id，则将 user1UnreadCount 设为 0
    // 如果 userName == user2Id，则将 user2UnreadCount 设为 0
    std::string sql =
        "UPDATE conversations "
        "SET user1UnreadCount = CASE WHEN user1Id = ? THEN 0 ELSE user1UnreadCount END, "
        "    user2UnreadCount = CASE WHEN user2Id = ? THEN 0 ELSE user2UnreadCount END "
        "WHERE convId = ?";

    try {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(sql));
        pstmt->setString(1, sql);
        pstmt->setString(1, userName); // 对应 user1Id
        pstmt->setString(2, userName); // 对应 user2Id
        pstmt->setString(3, convId);
        int affected = pstmt->executeUpdate();
        return affected;
    }
    catch (...) {
        return 0;
    }
}
std::vector<ChatRecord> ChatDao::getRecentChatRecordsBySessionId(const std::string& sessionId, int limit) const
{
    std::vector<ChatRecord> records;

    if (limit <= 0)
    {
        return records;
    }

    // 每次调用都创建一个独立连接
    auto con = Logger::GetInstance().createConnection();

    // 先查询该 sessionId 的总条数
    int totalCount = 0;
    try
    {
        std::string countSql = "SELECT COUNT(*) AS cnt FROM chatrecord WHERE sessionId = ?";
        std::unique_ptr<sql::PreparedStatement> countStmt(
            con->prepareStatement(countSql));
        countStmt->setString(1, sessionId);

        std::unique_ptr<sql::ResultSet> countRes(
            countStmt->executeQuery());
        if (countRes->next())
        {
            totalCount = countRes->getInt("cnt");
        }
    }
    catch (...)
    {
        // 如果统计失败，则退化为直接按传入的 limit 查询
        totalCount = 0;
    }

    // 实际要取的条数 = min(limit, totalCount)，
    // 如果 totalCount == 0，说明这个会话本来就没有记录
    int realLimit = limit;
    if (totalCount > 0 && limit > totalCount)
    {
        realLimit = totalCount;
    }

    // 如果没有记录，直接返回空 vector
    if (realLimit <= 0)
    {
        return records;
    }

    // 按 sendTime 降序取最近 realLimit 条
    std::string sql =
        "SELECT id, msgId, sendUserId, receiveType, receiveId, msgType, msgContent, "
        "       msgStatus, sendTime, readTime, extendInfo, sessionId "
        "FROM chatrecord "
        "WHERE sessionId = ? "
        "ORDER BY sendTime DESC "
        "LIMIT ?";

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(sql));
        pstmt->setString(1, sessionId);
        pstmt->setUInt(2, static_cast<unsigned int>(realLimit));

        std::unique_ptr<sql::ResultSet> res(
            pstmt->executeQuery());
        while (res->next())
        {
            ChatRecord rec;
            rec.setId(res->getUInt64("id"));
            rec.setMsgId(res->getUInt64("msgId"));
            rec.setSendUserId(res->getString("sendUserId"));
            rec.setReceiveType(static_cast<ChatRecord::ReceiveType>(res->getUInt("receiveType")));
            rec.setReceiveId(res->getString("receiveId"));
            rec.setMsgType(static_cast<ChatRecord::MsgType>(res->getUInt("msgType")));
            rec.setMsgContent(res->getString("msgContent"));
            rec.setMsgStatus(static_cast<ChatRecord::MsgStatus>(res->getUInt("msgStatus")));

            uint64_t sendTs = res->getUInt64("sendTime");
            rec.setSendTime(sendTs);

            uint64_t readTs = 0;
            try
            {
                readTs = res->getUInt64("readTime");
            }
            catch (...)
            {
                readTs = 0;
            }
            if (readTs != 0)
            {
                rec.setReadTime(readTs);
            }

            rec.setExtendInfo(static_cast<std::string>(res->getString("extendInfo")));
            rec.setSessionId(res->getString("sessionId"));

            records.push_back(std::move(rec));
        }
    }
    catch (...)
    {
        return records;
    }

    // 这里函数结束时：
    // res/pstmt/countRes/countStmt 先析构，再析构 con，连接自动关闭
    return records;
}
