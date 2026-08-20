#include "GroupMsgReadDao.h"

GroupMsgReadDao::GroupMsgReadDao() = default;

bool GroupMsgReadDao::markRead(const GroupMsgReadModel &model)
{
    try
    {
        auto con = Logger::GetInstance().createConnection();

        // 即使初始化未读记录遗漏，也能通过回执补建已读记录。
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "INSERT INTO groupMsgRead (msgId, userId, readTime) "
                "VALUES (?, ?, ?) "
                "ON DUPLICATE KEY UPDATE readTime = VALUES(readTime)"));

        // 优先使用服务层生成的时间，保证数据库与回执返回完全一致。
        uint64_t nowTs = model.getReadTime();
        if (nowTs == 0)
        {
            nowTs = Logger::GetInstance().getcurrentTime();
        }
        pstmt->setUInt64(1, model.getMsgId());
        pstmt->setString(2, model.getUserId());
        pstmt->setUInt64(3, nowTs);

        pstmt->executeUpdate();
        return true;
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return false;
}

std::vector<GroupMsgReadModel> GroupMsgReadDao::getReadStatusesByMsg(uint64_t msgId) const
{
    std::vector<GroupMsgReadModel> result;
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT id, msgId, userId, readTime "
                "FROM groupMsgRead "
                "WHERE msgId = ?"));

        pstmt->setUInt64(1, msgId);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            GroupMsgReadModel m;
            m.setId(res->getUInt64("id"));
            m.setMsgId(res->getUInt64("msgId"));
            m.setUserId(res->getString("userId"));
            m.setReadTime(res->getUInt64("readTime"));
            result.push_back(std::move(m));
        }
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return result;
}

std::vector<GroupMsgReadModel> GroupMsgReadDao::getReadersByMsg(uint64_t msgId) const
{
    std::vector<GroupMsgReadModel> result;
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT id, msgId, userId, readTime "
                "FROM groupMsgRead "
                "WHERE msgId = ? AND readTime > 0"));  // 只要已读记录

        pstmt->setUInt64(1, msgId);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            GroupMsgReadModel m;
            m.setId(res->getUInt64("id"));
            m.setMsgId(res->getUInt64("msgId"));
            m.setUserId(res->getString("userId"));
            m.setReadTime(res->getUInt64("readTime"));  // 时间戳
            result.push_back(std::move(m));
        }
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return result;
}

std::vector<GroupMsgReadModel> GroupMsgReadDao::getUserReadRecords(const std::string &userId,
                                                                    uint64_t msgIdBegin,
                                                                    uint64_t msgIdEnd) const
{
    std::vector<GroupMsgReadModel> result;
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT id, msgId, userId, readTime "
                "FROM groupMsgRead "
                "WHERE userId = ? AND msgId BETWEEN ? AND ? "
                "ORDER BY msgId ASC"));

        pstmt->setString(1, userId);                      // 改为 string
        pstmt->setUInt64(2, msgIdBegin);
        pstmt->setUInt64(3, msgIdEnd);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            GroupMsgReadModel m;
            m.setId(res->getUInt64("id"));
            m.setMsgId(res->getUInt64("msgId"));
            m.setUserId(res->getString("userId"));        // 改为 string
            m.setReadTime(res->getUInt64("readTime"));
            result.push_back(std::move(m));
        }
    }
    catch (const std::exception &e)
    {
        Logger::GetInstance().error(e.what());
    }
    return result;
}
bool GroupMsgReadDao::insert(GroupMsgReadModel& model)
{
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "INSERT INTO groupMsgRead (msgId, userId, readTime) "
                "VALUES (?, ?, ?) "
                "ON DUPLICATE KEY UPDATE readTime = VALUES(readTime)"));

        pstmt->setUInt64(1, model.getMsgId());
        pstmt->setString(2, model.getUserId());
        pstmt->setUInt64(3, model.getReadTime());   // 时间戳

        pstmt->executeUpdate();

        std::unique_ptr<sql::PreparedStatement> idStmt(
            con->prepareStatement("SELECT LAST_INSERT_ID() AS id"));
        std::unique_ptr<sql::ResultSet> res(idStmt->executeQuery());
        if (res->next())
        {
            model.setId(res->getUInt64("id"));
        }
        return true;
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return false;
}
bool GroupMsgReadDao::insertBatch(const std::vector<GroupMsgReadModel>& models)
{
    if (models.empty())
    {
        return true;
    }

    try
    {
        auto con = Logger::GetInstance().createConnection();

        // 构造一条批量 INSERT SQL：
        // INSERT INTO groupMsgRead (msgId, userId, readTime)
        // VALUES (?, ?, FROM_UNIXTIME(?)), (?, ?, FROM_UNIXTIME(?)), ...
        std::ostringstream oss;
        oss << "INSERT INTO groupMsgRead (msgId, userId, readTime) VALUES ";
        const std::size_t n = models.size();
        for (std::size_t i = 0; i < n; ++i)
        {
            if (i > 0)
            {
                oss << ", ";
            }
            oss << "(?, ?, ?)";
        }

        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(oss.str()));

        // 绑定参数
        // 对于第 i 条记录（0-based），其参数下标为：3*i+1, 3*i+2, 3*i+3
        for (std::size_t i = 0; i < n; ++i)
        {
            const auto& m = models[i];
            unsigned int base = static_cast<unsigned int>(3 * i + 1);
            pstmt->setUInt64(base + 0, m.getMsgId());
            pstmt->setString(base + 1, m.getUserId());
            pstmt->setUInt64(base + 2, m.getReadTime());
        }
        pstmt->executeUpdate();
        return true;
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return false;
}
bool GroupMsgReadDao::deleteByMsgId(uint64_t msgId)
{
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "DELETE FROM groupMsgRead WHERE msgId = ?"));

        pstmt->setUInt64(1, msgId);

        // 没有记录时 DELETE 也会返回 0 行，这里按你的 Dao 风格直接认为执行成功即可
        pstmt->executeUpdate();
        return true;
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return false;
}
