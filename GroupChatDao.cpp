#include "GroupChatDao.h"

GroupChatDao::GroupChatDao() = default;

uint64_t GroupChatDao::createGroup(const GroupChatModel& group)
{
    uint64_t newId = 0;
    try
    {
        auto con = Logger::GetInstance().createConnection();
        // 现在表里有 id（PK，自增）+ groupId（业务ID）
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "INSERT INTO groupChat "
                "(groupId, groupName, groupAvatar, creatorId, description, maxMembers, isActive, createdAt, updatedAt ) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"));

        pstmt->setUInt64(1, group.getGroupId());
        pstmt->setString(2, group.getGroupName());
        pstmt->setString(3, group.getGroupAvatar());
        pstmt->setString(4, group.getCreatorId());
        pstmt->setString(5, group.getDescription());
        pstmt->setUInt(6, group.getMaxMembers());
        pstmt->setUInt(7, group.getIsActive());
        pstmt->setUInt64(8, group.getCreatedAt());
        pstmt->setUInt64(9, group.getUpdatedAt());
        pstmt->executeUpdate();

        std::unique_ptr<sql::PreparedStatement> idStmt(
            con->prepareStatement("SELECT LAST_INSERT_ID() AS id"));
        std::unique_ptr<sql::ResultSet> res(idStmt->executeQuery());
        if (res->next())
        {
            newId = res->getUInt64("id");
        }
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return newId;
}

GroupChatModel GroupChatDao::getGroupById(uint64_t groupId) const
{
    GroupChatModel model;
    try
    {
        auto con = Logger::GetInstance().createConnection();
        // 按业务 groupId 查询；同时选出主键 id
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT id, groupId, groupName, groupAvatar, creatorId, "
                "description, maxMembers, isActive, "
                "createdAt, updatedAt "
                "FROM groupChat WHERE groupId = ?"));

        pstmt->setUInt64(1, groupId);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (res->next())
        {
            model.setId(res->getUInt64("id"));
            model.setGroupId(res->getUInt64("groupId"));
            model.setGroupName(res->getString("groupName"));
            model.setGroupAvatar(res->getString("groupAvatar"));
            model.setCreatorId(res->getString("creatorId"));
            model.setDescription(res->getString("description"));
            model.setMaxMembers(res->getUInt("maxMembers"));
            model.setIsActive(static_cast<uint8_t>(res->getUInt("isActive")));
            model.setCreatedAt(res->getUInt64("createdAtTs"));
            model.setUpdatedAt(res->getUInt64("updatedAtTs"));
        }
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return model;
}

std::vector<GroupChatModel> GroupChatDao::getGroupsByCreator(const std::string& creatorId) const
{
    std::vector<GroupChatModel> groups;
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT id, groupId, groupName, groupAvatar, creatorId, "
                "description, maxMembers, isActive, "
                "UNIX_TIMESTAMP(createdAt) AS createdAtTs, "
                "UNIX_TIMESTAMP(updatedAt) AS updatedAtTs "
                "FROM groupChat WHERE creatorId = ? ORDER BY createdAt DESC"));

        pstmt->setString(1, creatorId);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            GroupChatModel g;
            g.setId(res->getUInt64("id"));
            g.setGroupId(res->getUInt64("groupId"));
            g.setGroupName(res->getString("groupName"));
            g.setGroupAvatar(res->getString("groupAvatar"));
            g.setCreatorId(res->getString("creatorId"));
            g.setDescription(res->getString("description"));
            g.setMaxMembers(res->getUInt("maxMembers"));
            g.setIsActive(static_cast<uint8_t>(res->getUInt("isActive")));
            g.setCreatedAt(res->getUInt64("createdAtTs"));
            g.setUpdatedAt(res->getUInt64("updatedAtTs"));
            groups.push_back(std::move(g));
        }
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return groups;
}

bool GroupChatDao::updateGroupInfo(uint64_t groupId, const GroupChatModel& group)
{
    try
    {
        auto con = Logger::GetInstance().createConnection();

        std::ostringstream sql;
        sql << "UPDATE groupChat SET ";

        bool firstField = true;

        if (!group.getGroupName().empty())
        {
            sql << (firstField ? "" : ", ") << "groupName = ?";
            firstField = false;
        }
        if (!group.getGroupAvatar().empty())
        {
            sql << (firstField ? "" : ", ") << "groupAvatar = ?";
            firstField = false;
        }
        if (!group.getDescription().empty())
        {
            sql << (firstField ? "" : ", ") << "description = ?";
            firstField = false;
        }
        if (group.getMaxMembers() != 0)
        {
            sql << (firstField ? "" : ", ") << "maxMembers = ?";
            firstField = false;
        }
        if (group.getIsActive() != static_cast<uint8_t>(255))
        {
            sql << (firstField ? "" : ", ") << "isActive = ?";
            firstField = false;
        }

        if (firstField)
        {
            return true;
        }

        sql << " WHERE groupId = ?";

        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(sql.str()));

        unsigned int idx = 1;
        if (!group.getGroupName().empty())
        {
            pstmt->setString(idx++, group.getGroupName());
        }
        if (!group.getGroupAvatar().empty())
        {
            pstmt->setString(idx++, group.getGroupAvatar());
        }
        if (!group.getDescription().empty())
        {
            pstmt->setString(idx++, group.getDescription());
        }
        if (group.getMaxMembers() != 0)
        {
            pstmt->setUInt(idx++, group.getMaxMembers());
        }
        if (group.getIsActive() != static_cast<uint8_t>(255))
        {
            pstmt->setUInt(idx++, group.getIsActive());
        }

        pstmt->setUInt64(idx, groupId);

        return pstmt->executeUpdate() > 0;
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return false;
}

std::vector<GroupChatModel> GroupChatDao::getGroupsByUserId(const std::string& userId) const
{
    std::vector<GroupChatModel> groups;
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT gc.id, gc.groupId, gc.groupName, gc.groupAvatar, gc.creatorId, "
                "gc.description, gc.maxMembers, gc.isActive, "
                "gc.createdAt, gc.updatedAt "
                "FROM groupChat gc "
                "JOIN groupMember gm ON gc.groupId = gm.groupId "
                "WHERE gm.userId = ? AND gm.isQuit = 0"));

        pstmt->setString(1, userId);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next())
        {
            GroupChatModel g;
            g.setId(res->getUInt64("id"));
            g.setGroupId(res->getUInt64("groupId"));
            g.setGroupName(res->getString("groupName"));
            g.setGroupAvatar(res->getString("groupAvatar"));
            g.setCreatorId(res->getString("creatorId"));
            g.setDescription(res->getString("description"));
            g.setMaxMembers(res->getUInt("maxMembers"));
            g.setIsActive(static_cast<uint8_t>(res->getUInt("isActive")));
            g.setCreatedAt(res->getUInt64("createdAt"));   // 直接取 BIGINT
            g.setUpdatedAt(res->getUInt64("updatedAt"));
            groups.push_back(std::move(g));
        }
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return groups;
}

bool GroupChatDao::groupExists(uint64_t groupId) const
{
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT 1 FROM groupChat WHERE groupId = ? LIMIT 1"));

        pstmt->setUInt64(1, groupId);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        return res->next();
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return false;
}

bool GroupChatDao::deleteGroupById(uint64_t groupId)
{
    try
    {
        auto con = Logger::GetInstance().createConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement("DELETE FROM groupChat WHERE groupId = ?"));

        pstmt->setUInt64(1, groupId);

        // 受影响行数 > 0 认为删除成功
        return pstmt->executeUpdate() > 0;
    }
    catch (const std::exception& e)
    {
        Logger::GetInstance().error(e.what());
    }
    return false;
}
