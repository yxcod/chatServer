#include "GroupConversationDao.h"

GroupConversationDao::GroupConversationDao() = default;

// 需要在表上新增列并建唯一索引：
// ALTER TABLE groupConversations
//   ADD COLUMN msgType TINYINT UNSIGNED DEFAULT 0 COMMENT '消息类型：0-文本 1-图片 2-文件 3-语音',
//   ADD UNIQUE KEY uk_groupId (groupId);
int GroupConversationDao::insert(const GroupConversationModel& conv)
{
    int newId = 0;
    try
    {
        auto con = Logger::GetInstance().createConnection();
        // 一条 SQL：groupId 不存在则插入，存在则只更新最近消息相关字段
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "INSERT INTO groupConversations "
                "(groupId, updateTime, lastSenderId, lastMsg, validList, msgType) "
                "VALUES (?, ?, ?, ?, ?, ?) "
                "ON DUPLICATE KEY UPDATE "
                "updateTime = VALUES(updateTime), "
                "lastSenderId = VALUES(lastSenderId), "
                "lastMsg = VALUES(lastMsg), "
                "validList = VALUES(validList), "
                "msgType = VALUES(msgType)"));

        pstmt->setUInt64(1, conv.getGroupId());
        pstmt->setUInt64(2, conv.getUpdateTime());
        pstmt->setString(3, conv.getLastSenderId());
        pstmt->setString(4, conv.getLastMsg());
        pstmt->setString(5, conv.getValidList());
        pstmt->setUInt(6, conv.getMsgType());

        newId =  pstmt->executeUpdate();

        //std::unique_ptr<sql::PreparedStatement> idStmt(
        //    con->prepareStatement("SELECT LAST_INSERT_ID() AS id"));
        //std::unique_ptr<sql::ResultSet> res(idStmt->executeQuery());
        //if (res->next())
        //{
        //    newId = static_cast<int>(res->getUInt("id"));
        //}
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return newId;
}

GroupConversationModel GroupConversationDao::getByGroupId(uint64_t groupId) const
{
    GroupConversationModel model;
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT ID, groupId, updateTime, lastSenderId, lastMsg, validList, msgType "
                "FROM groupConversations WHERE groupId = ? LIMIT 1"));

        pstmt->setUInt64(1, groupId);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (res->next())
        {
            model.setId(static_cast<int>(res->getUInt("ID")));
            model.setGroupId(res->getUInt64("groupId"));
            model.setUpdateTime(res->getUInt64("updateTime"));
            model.setLastSenderId(res->getString("lastSenderId"));
            model.setLastMsg(res->getString("lastMsg"));
            model.setValidList(res->getString("validList"));
            model.setMsgType(static_cast<uint8_t>(res->getUInt("msgType")));
        }
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return model;
}

bool GroupConversationDao::upsert(const GroupConversationModel& conv)
{
    try
    {
        auto con = Logger::GetInstance().createConnection();
        // 先 UPDATE，不行再 INSERT（兼容旧代码）
        std::unique_ptr<sql::PreparedStatement> updateStmt(
            con->prepareStatement(
                "UPDATE groupConversations "
                "SET updateTime = ?, lastSenderId = ?, lastMsg = ?, "
                "validList = ?, msgType = ? "
                "WHERE groupId = ?"));

        updateStmt->setUInt64(1, conv.getUpdateTime());
        updateStmt->setString(2, conv.getLastSenderId());
        updateStmt->setString(3, conv.getLastMsg());
        updateStmt->setString(4, conv.getValidList());
        updateStmt->setUInt(5, conv.getMsgType());
        updateStmt->setUInt64(6, conv.getGroupId());

        int affected = updateStmt->executeUpdate();
        if (affected > 0)
        {
            return true;
        }

        return insert(conv) > 0;
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return false;
}

bool GroupConversationDao::updateLastMessage(uint64_t groupId,
    uint64_t updateTime,
    const std::string& lastSenderId,
    const std::string& lastMsg,
    const std::string& validList,
    uint8_t msgType)
{
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "UPDATE groupConversations "
                "SET updateTime = ?, lastSenderId = ?, lastMsg = ?, "
                "validList = ?, msgType = ? "
                "WHERE groupId = ?"));

        pstmt->setUInt64(1, updateTime);
        pstmt->setString(2, lastSenderId);
        pstmt->setString(3, lastMsg);
        pstmt->setString(4, validList);
        pstmt->setUInt(5, msgType);
        pstmt->setUInt64(6, groupId);

        return pstmt->executeUpdate() > 0;
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return false;
}

std::vector<GroupConversationModel> GroupConversationDao::getConversationsByUser(const std::string& userId) const
{
    std::vector<GroupConversationModel> result;
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT ID, groupId, updateTime, lastSenderId, lastMsg, validList, msgType "
                "FROM groupConversations "
                "WHERE validList LIKE ?"));

        std::string pattern = "%" + userId + "%";
        pstmt->setString(1, pattern);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            GroupConversationModel m;
            m.setId(static_cast<int>(res->getUInt("ID")));
            m.setGroupId(res->getUInt64("groupId"));
            m.setUpdateTime(res->getUInt64("updateTime"));
            m.setLastSenderId(res->getString("lastSenderId"));
            m.setLastMsg(res->getString("lastMsg"));
            m.setValidList(res->getString("validList"));
            m.setMsgType(static_cast<uint8_t>(res->getUInt("msgType")));
            result.push_back(std::move(m));
        }
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return result;
}
bool GroupConversationDao::deleteByGroupId(uint64_t groupId)
{
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "DELETE FROM groupConversations WHERE groupId = ?"));

        pstmt->setUInt64(1, groupId);

        // 受影响行数 > 0 表示删除成功
        return pstmt->executeUpdate() > 0;
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return false;
}