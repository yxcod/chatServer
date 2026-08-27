#include "FriendRelationDao.h"
#include "UserInfoDao.h"

namespace
{
void readOptionalFriendRegion(sql::ResultSet& result, UserInfo& userInfo)
{
    try
    {
        userInfo.setRegion(result.getString("region"));
    }
    catch (const std::exception& error)
    {
        userInfo.setRegion("");
        Logger::GetInstance().error(
            std::string("Friend region is unavailable; apply sql/user_profile_region.sql: ") +
            error.what());
    }
}
}

FriendRelationDao::FriendRelationDao()
{
}

void FriendRelationDao::getAllFriendWithUserId(const std::string& userId, const int& state, std::vector<UserInfo>& userInfoList) const
{
    std::string selectSql =
        "SELECT a.* FROM userinfo a "
        "JOIN friendrelation b "
        "  ON (b.fromUserId = ? AND a.userName = b.toUserId) "
        "  OR (b.toUserId = ? AND a.userName = b.fromUserId) "
        "WHERE b.status = ? AND a.userName != ?;";

    auto con = Logger::GetInstance().createConnection();

    std::vector<UserInfo> userInfoVector;
    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(selectSql));
        pstmt->setString(1, userId);
        pstmt->setString(2, userId);
        pstmt->setUInt(3, state);
        pstmt->setString(4, userId);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            UserInfo userInfo;
            userInfo.setUserId(res->getUInt64("userId"));
            userInfo.setUserAccount(res->getString("userName"));
            userInfo.setNickName(res->getString("nickName"));
            userInfo.setAvatar(res->getString("avatar"));
            userInfo.setGender(res->getUInt("gender"));
            readOptionalFriendRegion(*res, userInfo);
            userInfo.setSignature(res->getString("signature"));
            userInfo.setCreateTime(res->getUInt64("createTime"));
            userInfo.setState(res->getUInt("state"));
            userInfo.setModifyTime(res->getUInt64("modifyTime"));
            userInfoVector.push_back(userInfo);
        }
    }
    catch (const std::exception& error)
    {
        Logger::GetInstance().error(
            std::string("Failed to load friend list for ") + userId + ": " + error.what());
    }
    catch (...)
    {
        Logger::GetInstance().error(
            std::string("Failed to load friend list for ") + userId + ": unknown exception");
    }

    userInfoList = std::move(userInfoVector);
    return;
}

int FriendRelationDao::insertFriendApply(const FriendRelation& friendRelation) const
{
    auto con = Logger::GetInstance().createConnection();

    // Keep one application record for a directed user pair. A new attempt
    // refreshes the message and create time so expiry starts again.
    std::string selectSql =
        "SELECT id, status FROM friendrelation "
        "WHERE (fromUserId = ? AND toUserId = ?) "
        "LIMIT 1";

    std::unique_ptr<sql::PreparedStatement> checkStmt;
    std::unique_ptr<sql::ResultSet> res;

    try
    {
        checkStmt.reset(con->prepareStatement(selectSql));
        checkStmt->setString(1, friendRelation.getFromUserId());
        checkStmt->setString(2, friendRelation.getToUserId());

        res.reset(checkStmt->executeQuery());
        if (res->next())
        {
            const auto existingStatus = res->getUInt("status");
            int relationId = res->getInt("id");
            res.reset();

            if (existingStatus == static_cast<unsigned int>(
                    FriendRelation::RelationStatus::ACCEPTED))
            {
                return -2;
            }

            std::string updateSql =
                "UPDATE friendrelation SET status = ?, applyMsg = ?, "
                "source = ?, createTime = ?, updateTime = ? WHERE id = ?";
            std::unique_ptr<sql::PreparedStatement> updateStmt(
                con->prepareStatement(updateSql));
            updateStmt->setInt(1, 0); // 0 = 待验证
            updateStmt->setString(2, friendRelation.getApplyMsg());
            updateStmt->setString(3, friendRelation.getSource());
            updateStmt->setUInt64(4, friendRelation.getCreateTime());
            updateStmt->setUInt64(5, friendRelation.getUpdateTime());
            updateStmt->setInt(6, relationId);
            return updateStmt->executeUpdate();
        }
    }
    catch (...)
    {
        return 0;
    }

    // 不存在记录：执行插入
    std::string insertSql =
        "INSERT INTO friendrelation "
        "(fromUserId, toUserId, status, fromRemark, toRemark, source, applyMsg, createTime, updateTime) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(insertSql));
        pstmt->setString(1, friendRelation.getFromUserId());
        pstmt->setString(2, friendRelation.getToUserId());
        pstmt->setInt(3, 0);
        pstmt->setString(4, friendRelation.getFromRemark());
        pstmt->setString(5, friendRelation.getToRemark());
        pstmt->setString(6, friendRelation.getSource());
        pstmt->setString(7, friendRelation.getApplyMsg());
        pstmt->setUInt64(8, friendRelation.getCreateTime());
        pstmt->setUInt64(9, friendRelation.getUpdateTime());
        return pstmt->executeUpdate();
    }
    catch (...)
    {
        return 0;
    }
}

int FriendRelationDao::updateFriendApplyStatus(const int& relationId, const int& state) const
{
    if (state != static_cast<int>(FriendRelation::RelationStatus::ACCEPTED) &&
        state != static_cast<int>(FriendRelation::RelationStatus::REJECTED))
    {
        return 0;
    }

    const uint64_t now = Logger::GetInstance().getcurrentTime();
    const uint64_t expiryWindow = 3ULL * 24ULL * 60ULL * 60ULL * 1000ULL;
    const uint64_t cutoff = now > expiryWindow ? now - expiryWindow : 0;
    std::string updateSql =
        "UPDATE friendrelation SET status = ?, updateTime = ? "
        "WHERE id = ? AND status = 0 AND createTime >= ?";

    auto con = Logger::GetInstance().createConnection();

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(updateSql));
        pstmt->setInt(1, state);
        pstmt->setUInt64(2, now);
        pstmt->setInt(3, relationId);
        pstmt->setUInt64(4, cutoff);
        return pstmt->executeUpdate();
    }
    catch (...)
    {
        return 0;
    }
}

int FriendRelationDao::deleteExpiredFriendApply(
    uint64_t relationId, const std::string& userId) const
{
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
            "DELETE FROM friendrelation WHERE id = ? AND status = 6 "
            "AND (fromUserId = ? OR toUserId = ?)"));
        pstmt->setUInt64(1, relationId);
        pstmt->setString(2, userId);
        pstmt->setString(3, userId);
        return pstmt->executeUpdate();
    }
    catch (const std::exception& error)
    {
        Logger::GetInstance().error(
            std::string("Failed to delete expired friend application: ") +
            error.what());
        return 0;
    }
}

int FriendRelationDao::deleteFriendRelation(const std::string& userId1, const std::string& userId2) const
{
    std::string updateSql =
        "UPDATE friendrelation "
        "SET status = ?, updateTime = ? "
        "WHERE (fromUserId = ? AND toUserId = ?) "
        "   OR (fromUserId = ? AND toUserId = ?);";

    auto con = Logger::GetInstance().createConnection();
    const uint64_t now = Logger::GetInstance().getcurrentTime();

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(updateSql));
        pstmt->setInt(1, 5);      // status = 5 删除
        pstmt->setUInt64(2, now); // 更新时间
        pstmt->setString(3, userId1);
        pstmt->setString(4, userId2);
        pstmt->setString(5, userId2);
        pstmt->setString(6, userId1);
        return pstmt->executeUpdate();
    }
    catch (...)
    {
        return 0;
    }
}

std::vector<FriendRelation> FriendRelationDao::getFriendApplyListForUser(
    const std::string& userId, const uint64_t& nowTs) const
{
    std::vector<FriendRelation> relations;
    const uint64_t expiryWindow = 3ULL * 24ULL * 60ULL * 60ULL * 1000ULL;
    const uint64_t cutoff = nowTs > expiryWindow ? nowTs - expiryWindow : 0;

    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> expireStmt(
            con->prepareStatement(
                "UPDATE friendrelation SET status = 6, updateTime = ? "
                "WHERE (fromUserId = ? OR toUserId = ?) "
                "AND status = 0 AND createTime < ?"));
        expireStmt->setUInt64(1, nowTs);
        expireStmt->setString(2, userId);
        expireStmt->setString(3, userId);
        expireStmt->setUInt64(4, cutoff);
        expireStmt->executeUpdate();

        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT * FROM friendrelation "
                "WHERE (fromUserId = ? OR toUserId = ?) "
                "AND status IN (0, 6) "
                "ORDER BY updateTime DESC, id DESC LIMIT 100"));
        pstmt->setString(1, userId);
        pstmt->setString(2, userId);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        while (res->next())
        {
            FriendRelation relation;
            relation.setId(res->getUInt64("id"));
            relation.setFromUserId(res->getString("fromUserId"));
            relation.setToUserId(res->getString("toUserId"));
            relation.setStatus(static_cast<uint8_t>(res->getUInt("status")));
            relation.setFromRemark(res->getString("fromRemark"));
            relation.setToRemark(res->getString("toRemark"));
            relation.setSource(res->getString("source"));
            relation.setApplyMsg(res->getString("applyMsg"));
            relation.setCreateTime(res->getUInt64("createTime"));
            relation.setUpdateTime(res->getUInt64("updateTime"));
            relations.push_back(relation);
        }
    }
    catch (const std::exception& error)
    {
        Logger::GetInstance().error(
            std::string("Failed to load friend applications: ") + error.what());
    }
    return relations;
}

FriendRelation FriendRelationDao::getFriendRelationById(uint64_t relationId) const
{
    FriendRelation relation;
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement("SELECT * FROM friendrelation WHERE id = ? LIMIT 1"));
        pstmt->setUInt64(1, relationId);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res->next()) return relation;
        relation.setId(res->getUInt64("id"));
        relation.setFromUserId(res->getString("fromUserId"));
        relation.setToUserId(res->getString("toUserId"));
        relation.setStatus(static_cast<uint8_t>(res->getUInt("status")));
        relation.setFromRemark(res->getString("fromRemark"));
        relation.setToRemark(res->getString("toRemark"));
        relation.setSource(res->getString("source"));
        relation.setApplyMsg(res->getString("applyMsg"));
        relation.setCreateTime(res->getUInt64("createTime"));
        relation.setUpdateTime(res->getUInt64("updateTime"));
    }
    catch (const std::exception& error)
    {
        Logger::GetInstance().error(
            std::string("Failed to load friend application by id: ") + error.what());
    }
    return relation;
}

FriendRelation FriendRelationDao::getDirectedFriendRelation(
    const std::string& fromUserId, const std::string& toUserId) const
{
    FriendRelation relation;
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
            "SELECT * FROM friendrelation WHERE fromUserId = ? AND toUserId = ? "
            "ORDER BY id DESC LIMIT 1"));
        pstmt->setString(1, fromUserId);
        pstmt->setString(2, toUserId);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res->next()) return relation;
        relation.setId(res->getUInt64("id"));
        relation.setFromUserId(res->getString("fromUserId"));
        relation.setToUserId(res->getString("toUserId"));
        relation.setStatus(static_cast<uint8_t>(res->getUInt("status")));
        relation.setFromRemark(res->getString("fromRemark"));
        relation.setToRemark(res->getString("toRemark"));
        relation.setSource(res->getString("source"));
        relation.setApplyMsg(res->getString("applyMsg"));
        relation.setCreateTime(res->getUInt64("createTime"));
        relation.setUpdateTime(res->getUInt64("updateTime"));
    }
    catch (const std::exception& error)
    {
        Logger::GetInstance().error(
            std::string("Failed to load directed friend application: ") + error.what());
    }
    return relation;
}

FriendRelation FriendRelationDao::getFriendRelation(const std::string& userId1, const std::string& userId2) const
{
    FriendRelation relation;

    std::string selectSql =
        "SELECT * FROM friendrelation "
        "WHERE (fromUserId = ? AND toUserId = ?) "
        "   OR (fromUserId = ? AND toUserId = ?) "
        "LIMIT 1";

    auto con = Logger::GetInstance().createConnection();

    std::unique_ptr<sql::ResultSet> res;

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(selectSql));
        pstmt->setString(1, userId1);
        pstmt->setString(2, userId2);
        pstmt->setString(3, userId2);
        pstmt->setString(4, userId1);

        res.reset(pstmt->executeQuery());
        if (res->next())
        {
            relation.setId(res->getUInt64("id"));
            relation.setFromUserId(res->getString("fromUserId"));
            relation.setToUserId(res->getString("toUserId"));
            relation.setStatus(static_cast<uint8_t>(res->getUInt("status")));
            relation.setFromRemark(res->getString("fromRemark"));
            relation.setToRemark(res->getString("toRemark"));
            relation.setSource(res->getString("source"));
            relation.setApplyMsg(res->getString("applyMsg"));
            relation.setCreateTime(res->getUInt64("createTime"));
            relation.setUpdateTime(res->getUInt64("updateTime"));
        }
    }
    catch (...)
    {
        return relation;
    }

    return relation;
}

bool FriendRelationDao::hasAcceptedRelation(const std::string& userId1, const std::string& userId2) const
{
    if (userId1.empty() || userId2.empty() || userId1 == userId2)
    {
        return false;
    }

    const std::string selectSql =
        "SELECT 1 FROM friendrelation "
        "WHERE ((fromUserId = ? AND toUserId = ?) "
        "    OR (fromUserId = ? AND toUserId = ?)) "
        "  AND status = ? "
        "LIMIT 1";

    try
    {
        auto con = Logger::GetInstance().createConnection();
        if (!con)
        {
            Logger::GetInstance().error("Unable to verify friend relation: database connection is null");
            return false;
        }

        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(selectSql));
        pstmt->setString(1, userId1);
        pstmt->setString(2, userId2);
        pstmt->setString(3, userId2);
        pstmt->setString(4, userId1);
        pstmt->setUInt(5, static_cast<unsigned int>(FriendRelation::RelationStatus::ACCEPTED));
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        return res && res->next();
    }
    catch (const std::exception& error)
    {
        Logger::GetInstance().error(
            std::string("Unable to verify friend relation: ") + error.what());
    }
    catch (...)
    {
        Logger::GetInstance().error("Unable to verify friend relation: unknown exception");
    }
    return false;
}

std::string FriendRelationDao::getFriendRemark(const std::string& userId1, const std::string& userId2) const
{
    std::string remark;

    std::string selectSql =
        "SELECT CASE "
        "         WHEN fromUserId = ? AND toUserId = ? THEN toRemark "
        "         WHEN fromUserId = ? AND toUserId = ? THEN fromRemark "
        "       END AS remark "
        "FROM friendrelation "
        "WHERE (fromUserId = ? AND toUserId = ?) "
        "   OR (fromUserId = ? AND toUserId = ?) "
        "LIMIT 1";

    auto con = Logger::GetInstance().createConnection();

    std::unique_ptr<sql::ResultSet> res;

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(selectSql));
        // CASE 部分参数
        pstmt->setString(1, userId1);
        pstmt->setString(2, userId2);
        pstmt->setString(3, userId2);
        pstmt->setString(4, userId1);
        // WHERE 部分参数
        pstmt->setString(5, userId1);
        pstmt->setString(6, userId2);
        pstmt->setString(7, userId2);
        pstmt->setString(8, userId1);

        res.reset(pstmt->executeQuery());
        if (res->next())
        {
            remark = res->getString("remark");
        }
    }
    catch (...)
    {
        return remark;
    }

    return remark;
}

int FriendRelationDao::updateFriendRemark(const std::string& userId1, const std::string& userId2, const std::string& remark) const
{
    std::string updateSql =
        "UPDATE friendrelation "
        "SET "
        "  toRemark = CASE "
        "               WHEN fromUserId = ? AND toUserId = ? THEN ? "
        "               ELSE toRemark "
        "             END, "
        "  fromRemark = CASE "
        "                 WHEN fromUserId = ? AND toUserId = ? THEN ? "
        "                 ELSE fromRemark "
        "               END, "
        "  updateTime = ? "
        "WHERE (fromUserId = ? AND toUserId = ?) "
        "   OR (fromUserId = ? AND toUserId = ?);";

    auto con = Logger::GetInstance().createConnection();
    const uint64_t now = Logger::GetInstance().getcurrentTime();

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(updateSql));
        // for toRemark
        pstmt->setString(1, userId1);
        pstmt->setString(2, userId2);
        pstmt->setString(3, remark);
        // for fromRemark
        pstmt->setString(4, userId2);
        pstmt->setString(5, userId1);
        pstmt->setString(6, remark);
        // updateTime
        pstmt->setUInt64(7, now);
        // WHERE
        pstmt->setString(8, userId1);
        pstmt->setString(9, userId2);
        pstmt->setString(10, userId2);
        pstmt->setString(11, userId1);

        return pstmt->executeUpdate();
    }
    catch (...)
    {
        return 0;
    }
}

std::vector<FriendRelation> FriendRelationDao::getRecentFriendApplyByUser(const std::string& userName, const uint64_t& nowTs) const
{
    std::vector<FriendRelation> relations;

    const uint64_t recentWindow = 3ULL * 24ULL * 60ULL * 60ULL * 1000ULL;
    const uint64_t cutoff = nowTs > recentWindow ? nowTs - recentWindow : 0;

    std::string selectSql =
        "SELECT * FROM friendrelation "
        "WHERE (fromUserId = ? OR toUserId = ?) "
        "  AND status IN (1, 2) "
        "  AND updateTime >= ? "
        "ORDER BY updateTime DESC, id DESC LIMIT 50";

    auto con = Logger::GetInstance().createConnection();

    std::unique_ptr<sql::ResultSet> res;

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(selectSql));
        pstmt->setString(1, userName);
        pstmt->setString(2, userName);
        pstmt->setUInt64(3, cutoff);

        res.reset(pstmt->executeQuery());
        while (res->next())
        {
            FriendRelation relation;
            relation.setId(res->getUInt64("id"));
            relation.setFromUserId(res->getString("fromUserId"));
            relation.setToUserId(res->getString("toUserId"));
            relation.setStatus(static_cast<uint8_t>(res->getUInt("status")));
            relation.setFromRemark(res->getString("fromRemark"));
            relation.setToRemark(res->getString("toRemark"));
            relation.setSource(res->getString("source"));
            relation.setApplyMsg(res->getString("applyMsg"));
            relation.setCreateTime(res->getUInt64("createTime"));
            relation.setUpdateTime(res->getUInt64("updateTime"));

            relations.push_back(relation);
        }
    }
    catch (...)
    {
        return relations;
    }

    return relations;
}
