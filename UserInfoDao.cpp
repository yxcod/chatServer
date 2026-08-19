#include "UserInfoDao.h"
#include "Logger.h"

// userId, userName, nickName, avatar, gender, signature, createTime, state
UserInfoDao::UserInfoDao()
{
    userInfoKey[UserInfoValueType::userId] = "userId";
    userInfoKey[UserInfoValueType::userName] = "userName";
    userInfoKey[UserInfoValueType::nickName] = "nickName";
    userInfoKey[UserInfoValueType::avatar] = "avatar";
    userInfoKey[UserInfoValueType::gender] = "gender";
    userInfoKey[UserInfoValueType::signature] = "signature";
    userInfoKey[UserInfoValueType::createTime] = "createTime";
    userInfoKey[UserInfoValueType::state] = "state";
    userInfoKey[UserInfoValueType::all] = "*";
}

int UserInfoDao::insertUserInfo(const UserInfo& userInfo)
{
    std::string insertSql =
        "INSERT INTO userinfo "
        "(userName, nickName, avatar, gender, signature, createTime, state, modifyTime) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)";

    auto con = Logger::GetInstance().createConnection();

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(insertSql));
        pstmt->setString(1, userInfo.getUserAccount());
        pstmt->setString(2, userInfo.getNickName());
        pstmt->setString(3, userInfo.getAvatar());
        pstmt->setUInt(4, userInfo.getGender());
        pstmt->setString(5, userInfo.getSignature());
        pstmt->setUInt64(6, userInfo.getCreateTime());
        pstmt->setInt(7, userInfo.getState());
        pstmt->setInt(8, userInfo.getModifyTime());
        return pstmt->executeUpdate();
    }
    catch (...)
    {
        return 0;
    }
}

UserInfo UserInfoDao::getUserinfo(const std::string& userId) const
{
    std::string selectSql = "SELECT * FROM userinfo WHERE userName = ?";

    auto con = Logger::GetInstance().createConnection();
    UserInfo userInfo;

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(selectSql));
        pstmt->setString(1, userId);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (res->next())
        {
            userInfo.setUserId(res->getUInt64("userId"));
            userInfo.setUserAccount(res->getString("userName"));
            userInfo.setNickName(res->getString("nickName"));
            userInfo.setAvatar(res->getString("avatar"));
            userInfo.setGender(res->getUInt("gender"));
            userInfo.setSignature(res->getString("signature"));
            userInfo.setCreateTime(res->getUInt64("createTime"));
            userInfo.setState(res->getUInt("state"));
            userInfo.setModifyTime(res->getUInt64("modifyTime"));
        }
    }
    catch (...)
    {
        // 保持默认 userInfo 返回
    }

    return userInfo;
}

int UserInfoDao::updateUserInfo(const std::string& userId, const UserInfo& userInfo)
{
    const std::string updateSql =
        "UPDATE userinfo SET "
        "nickName = CASE WHEN ? <> '' THEN ? ELSE nickName END, "
        "avatar = CASE WHEN ? <> '' THEN ? ELSE avatar END, "
        "gender = CASE WHEN ? <> 0 THEN ? ELSE gender END, "
        "signature = CASE WHEN ? <> '' THEN ? ELSE signature END, "
        "state = CASE WHEN ? <> 2 THEN ? ELSE state END, "
        "modifyTime = ? "
        "WHERE userName = ?";

    auto con = Logger::GetInstance().createConnection();

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(updateSql));

        const std::string nick = userInfo.getNickName();
        pstmt->setString(1, nick);
        pstmt->setString(2, nick);

        const std::string avatar = userInfo.getAvatar();
        pstmt->setString(3, avatar);
        pstmt->setString(4, avatar);

        const unsigned int gender = userInfo.getGender();
        pstmt->setUInt(5, gender);
        pstmt->setUInt(6, gender);

        const std::string sig = userInfo.getSignature();
        pstmt->setString(7, sig);
        pstmt->setString(8, sig);

        const unsigned int state = userInfo.getState();
        pstmt->setUInt(9, state);
        pstmt->setUInt(10, state);

        const uint64_t now = Logger::GetInstance().getcurrentTime();
        pstmt->setUInt64(11, now);

        pstmt->setString(12, userId);

        return pstmt->executeUpdate();
    }
    catch (...)
    {
        return 0;
    }
}

UserInfo UserInfoDao::getUserValueWithType(const UserInfoValueType& type, const std::string& userId) const
{
    std::string selectSql =
        "SELECT " + userInfoKey.at(type) + " FROM userinfo WHERE userName = ?";

    auto con = Logger::GetInstance().createConnection();
    UserInfo userInfo;

    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(selectSql));
        pstmt->setString(1, userId);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        if (!res->next())
        {
            return userInfo;
        }

        switch (type)
        {
        case UserInfoValueType::userId:
            userInfo.setUserId(res->getUInt64("userId"));
            break;
        case UserInfoValueType::userName:
            userInfo.setUserAccount(res->getString("userName"));
            break;
        case UserInfoValueType::nickName:
            userInfo.setNickName(res->getString("nickName"));
            break;
        case UserInfoValueType::avatar:
            userInfo.setAvatar(res->getString("avatar"));
            break;
        case UserInfoValueType::gender:
            userInfo.setGender(res->getUInt("gender"));
            break;
        case UserInfoValueType::signature:
            userInfo.setSignature(res->getString("signature"));
            break;
        case UserInfoValueType::createTime:
            userInfo.setCreateTime(res->getUInt64("createTime"));
            break;
        case UserInfoValueType::state:
            userInfo.setState(res->getUInt("state"));
            break;
        case UserInfoValueType::all:
            userInfo.setUserId(res->getUInt64("userId"));
            userInfo.setUserAccount(res->getString("userName"));
            userInfo.setNickName(res->getString("nickName"));
            userInfo.setAvatar(res->getString("avatar"));
            userInfo.setGender(res->getUInt("gender"));
            userInfo.setSignature(res->getString("signature"));
            userInfo.setCreateTime(res->getUInt64("createTime"));
            userInfo.setState(res->getUInt("state"));
            break;
        default:
            break;
        }
    }
    catch (...)
    {
        // 出错返回默认 userInfo
    }

    return userInfo;
}