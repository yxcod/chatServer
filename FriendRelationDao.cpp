#include "FriendRelationDao.h"
#include "UserInfoDao.h"

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
            userInfo.setSignature(res->getString("signature"));
            userInfo.setCreateTime(res->getUInt64("createTime"));
            userInfo.setState(res->getUInt("state"));
            userInfoVector.push_back(userInfo);
        }
    }
    catch (...)
    {
        // 出错时直接返回当前已收集的数据
    }

    userInfoList = std::move(userInfoVector);
    return;
}

int FriendRelationDao::insertFriendApply(const FriendRelation& friendRelation) const
{
    auto con = Logger::GetInstance().createConnection();

    // 先判断是否已存在这两个用户的好友关系
    std::string selectSql =
        "SELECT id FROM friendrelation "
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
            // 已存在记录：只更新 status = 0 和 updateTime
            int relationId = res->getInt("id");
            res.reset();

            std::string updateSql =
                "UPDATE friendrelation SET status = ?, updateTime = ? WHERE id = ?";
            std::unique_ptr<sql::PreparedStatement> updateStmt(
                con->prepareStatement(updateSql));
            updateStmt->setInt(1, 0); // 0 = 待验证
            updateStmt->setUInt64(2, Logger::GetInstance().getcurrentTime());
            updateStmt->setInt(3, relationId);
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
    std::string updateSql =
        "UPDATE friendrelation SET status = ?, updateTime = ? WHERE id = ?";

    auto con = Logger::GetInstance().createConnection();

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(updateSql));
        pstmt->setInt(1, state);
        pstmt->setUInt64(2, Logger::GetInstance().getcurrentTime());
        pstmt->setInt(3, relationId);
        return pstmt->executeUpdate();
    }
    catch (...)
    {
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

std::vector<FriendRelation> FriendRelationDao::getToUseridFriendApplyList(const std::string& userId) const
{
    std::vector<FriendRelation> userInfoVector;

    std::string selectSql =
        "SELECT * FROM friendrelation WHERE toUserId = ? AND status = 0;";

    auto con = Logger::GetInstance().createConnection();

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(selectSql));
        pstmt->setString(1, userId);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        UserInfoDao userInfoDao;
        while (res->next())
        {
            FriendRelation friendRelation;
            friendRelation.setId(res->getInt("id"));
            friendRelation.setCreateTime(res->getUInt64("createTime"));
            friendRelation.setFromUserId(res->getString("fromUserId"));
            friendRelation.setToUserId(res->getString("toUserId"));
            friendRelation.setApplyMsg(res->getString("applyMsg"));

            UserInfo userInfo = userInfoDao.getUserinfo(res->getString("fromUserId"));
            friendRelation.setToUserId(userInfo.getNickName());

            userInfoVector.push_back(friendRelation);
        }
    }
    catch (...)
    {
        return userInfoVector;
    }

    return userInfoVector;
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

    const uint64_t tenDaysMs = 10ULL * 24ULL * 60ULL * 60ULL * 1000ULL;
    uint64_t beginTs = nowTs > tenDaysMs ? nowTs - tenDaysMs : 0;

    std::string selectSql =
        "SELECT * FROM friendrelation "
        "WHERE toUserId = ? "
        "  AND status = 1 "
        "  AND createTime >= ? "
        "  AND createTime <= ?";

    auto con = Logger::GetInstance().createConnection();

    std::unique_ptr<sql::ResultSet> res;

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(selectSql));
        pstmt->setString(1, userName);
        pstmt->setUInt64(2, beginTs);
        pstmt->setUInt64(3, nowTs);

        res.reset(pstmt->executeQuery());
        while (res->next())
        {
            FriendRelation relation;
            relation.setId(res->getUInt64("id"));
            relation.setFromUserId(res->getString("fromUserId"));
            relation.setToUserId(res->getString("toUserId"));
            // relation.setStatus(static_cast<uint8_t>(res->getUInt("status")));
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