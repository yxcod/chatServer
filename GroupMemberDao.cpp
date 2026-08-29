#include "GroupMemberDao.h"

GroupMemberDao::GroupMemberDao() = default;

bool GroupMemberDao::addMember(GroupMemberModel& member)
{
    try
    {
        auto con = Logger::GetInstance().createConnection();

        // (groupId, userId) 唯一：不存在则插入；存在则只把 isQuit 置为 0
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "INSERT INTO groupMember "
                "(groupId, userId, role, joinTime, quitTime, isQuit, groupNickName, "
                "isMuted, mutedBy, mutedAt) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
                "ON DUPLICATE KEY UPDATE "
                "role = VALUES(role), joinTime = VALUES(joinTime), quitTime = 0, "
                "isQuit = 0, groupNickName = VALUES(groupNickName), "
                "isMuted = 0, mutedBy = '', mutedAt = 0"));

        pstmt->setUInt64(1, member.getGroupId());
        pstmt->setString(2, member.getUserId());
        pstmt->setUInt(3, member.getRole());
        // 这里直接写时间戳（BIGINT）
        pstmt->setUInt64(4, member.getJoinTime());
        pstmt->setUInt64(5, member.getQuitTime());   // 未退出可约定为 0
        pstmt->setUInt(6, member.getIsQuit());
        pstmt->setString(7, member.getGroupNickName());
        pstmt->setUInt(8, member.getIsMuted());
        pstmt->setString(9, member.getMutedBy());
        pstmt->setUInt64(10, member.getMutedAt());

        pstmt->executeUpdate();

        std::unique_ptr<sql::PreparedStatement> idStmt(
            con->prepareStatement("SELECT LAST_INSERT_ID() AS id"));
        std::unique_ptr<sql::ResultSet> res(idStmt->executeQuery());
        if (res->next())
        {
            auto id = res->getUInt64("id");
            if (id != 0)
            {
                member.setId(id);
            }
        }
        return true;
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return false;
}

std::vector<GroupMemberModel> GroupMemberDao::getMembersByGroup(uint64_t groupId) const
{
    std::vector<GroupMemberModel> members;
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT id, groupId, userId, role, "
                "joinTime, quitTime, "
                "isQuit, groupNickName, isMuted, mutedBy, mutedAt "
                "FROM groupMember WHERE groupId = ? AND isQuit = 0"));

        pstmt->setUInt64(1, groupId);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            GroupMemberModel m;
            m.setId(res->getUInt64("id"));
            m.setGroupId(res->getUInt64("groupId"));
            m.setUserId(res->getString("userId"));
            m.setRole(static_cast<uint8_t>(res->getUInt("role")));
            m.setJoinTime(res->getUInt64("joinTime"));   // 直接取 BIGINT
            m.setQuitTime(res->getUInt64("quitTime"));
            m.setIsQuit(static_cast<uint8_t>(res->getUInt("isQuit")));
            m.setGroupNickName(res->getString("groupNickName"));
            m.setIsMuted(static_cast<uint8_t>(res->getUInt("isMuted")));
            m.setMutedBy(res->getString("mutedBy"));
            m.setMutedAt(res->getUInt64("mutedAt"));
            members.push_back(std::move(m));
        }
    }
    catch (const std::exception& e)
    {
       // Logger::GetInstance().error(e.what > ());
    }
    return members;
}

std::vector<GroupMemberModel> GroupMemberDao::getGroupsByUser(const std::string& userId) const
{
    std::vector<GroupMemberModel> result;
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT id, groupId, userId, role, "
                "joinTime, quitTime, "
                "isQuit, groupNickName, isMuted, mutedBy, mutedAt "
                "FROM groupMember "
                "WHERE userId = ? AND isQuit = 0"));

        pstmt->setString(1, userId);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            GroupMemberModel m;
            m.setId(res->getUInt64("id"));
            m.setGroupId(res->getUInt64("groupId"));
            m.setUserId(res->getString("userId"));
            m.setRole(static_cast<uint8_t>(res->getUInt("role")));
            m.setJoinTime(res->getUInt64("joinTime"));
            m.setQuitTime(res->getUInt64("quitTime"));
            m.setIsQuit(static_cast<uint8_t>(res->getUInt("isQuit")));
            m.setGroupNickName(res->getString("groupNickName"));
            m.setIsMuted(static_cast<uint8_t>(res->getUInt("isMuted")));
            m.setMutedBy(res->getString("mutedBy"));
            m.setMutedAt(res->getUInt64("mutedAt"));
            result.push_back(std::move(m));
        }
    }
    catch (const std::exception& e)
    {
        //Logger::GetInstance().error(e.what > ());
    }
    return result;
}

bool GroupMemberDao::markQuit(uint64_t groupId, const std::string& userId, uint64_t quitTime)
{
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "UPDATE groupMember "
                "SET quitTime = ?, isQuit = 1 "
                "WHERE groupId = ? AND userId = ? AND isQuit = 0"));

        pstmt->setUInt64(1, quitTime);   // BIGINT 时间戳
        pstmt->setUInt64(2, groupId);
        pstmt->setString(3, userId);

        return pstmt->executeUpdate() > 0;
    }
    catch (const std::exception& e)
    {
        //Logger::GetInstance().error(e.what > ());
    }
    return false;
}

bool GroupMemberDao::isUserInGroup(uint64_t groupId, const std::string& userId) const
{
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                // 如需排除已退出成员，可改为: WHERE groupId = ? AND userId = ? AND isQuit = 0
                "SELECT 1 FROM groupMember "
                "WHERE groupId = ? AND userId = ? AND isQuit = 0 LIMIT 1"));

        pstmt->setUInt64(1, groupId);
        pstmt->setString(2, userId);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        // 存在一行即表示在群里
        return res->next();
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return false;
}

std::optional<uint8_t> GroupMemberDao::getActiveMemberRole(
    uint64_t groupId,
    const std::string& userId) const
{
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT role FROM groupMember "
                "WHERE groupId = ? AND userId = ? AND isQuit = 0 LIMIT 1"));
        pstmt->setUInt64(1, groupId);
        pstmt->setString(2, userId);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res->next()) return std::nullopt;
        return static_cast<uint8_t>(res->getUInt("role"));
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(
            std::string("getActiveMemberRole failed: ") + e.what());
    }
    return std::nullopt;
}
// 更新用户在群内的昵称
bool GroupMemberDao::updateGroupNickName(uint64_t groupId,
    const std::string& userId,
    const std::string& groupNickName)
{
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "UPDATE groupMember "
                "SET groupNickName = ? "
                "WHERE groupId = ? AND userId = ?"));

        pstmt->setString(1, groupNickName);
        pstmt->setUInt64(2, groupId);
        pstmt->setString(3, userId);

        return pstmt->executeUpdate() > 0;
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return false;
}

// 更新用户在群内的身份
bool GroupMemberDao::updateGroupRole(uint64_t groupId,
    const std::string& userId,
    uint8_t role)
{
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "UPDATE groupMember "
                "SET role = ? "
                "WHERE groupId = ? AND userId = ? AND isQuit = 0"));

        pstmt->setUInt(1, role);
        pstmt->setUInt64(2, groupId);
        pstmt->setString(3, userId);

        return pstmt->executeUpdate() > 0;
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return false;
}

bool GroupMemberDao::updateMuteState(uint64_t groupId,
    const std::string& userId,
    bool isMuted,
    const std::string& operatorId,
    uint64_t updatedAt)
{
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "UPDATE groupMember SET isMuted = ?, mutedBy = ?, mutedAt = ? "
                "WHERE groupId = ? AND userId = ? AND isQuit = 0"));
        pstmt->setUInt(1, isMuted ? 1 : 0);
        pstmt->setString(2, isMuted ? operatorId : "");
        pstmt->setUInt64(3, isMuted ? updatedAt : 0);
        pstmt->setUInt64(4, groupId);
        pstmt->setString(5, userId);
        return pstmt->executeUpdate() > 0;
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return false;
}

bool GroupMemberDao::isMemberMuted(
    uint64_t groupId, const std::string& userId) const
{
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT isMuted FROM groupMember "
                "WHERE groupId = ? AND userId = ? AND isQuit = 0 LIMIT 1"));
        pstmt->setUInt64(1, groupId);
        pstmt->setString(2, userId);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        return res->next() && res->getUInt("isMuted") != 0;
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return false;
}
bool GroupMemberDao::deleteByGroupId(uint64_t groupId)
{
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "DELETE FROM groupMember WHERE groupId = ?"));

        pstmt->setUInt64(1, groupId);

        // 受影响行数 >= 0 都认为成功（如果没有行也不算错误）
        pstmt->executeUpdate();
        return true;
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return false;
}
