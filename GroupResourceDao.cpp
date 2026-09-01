#include "GroupResourceDao.h"

#include <memory>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>

#include "DatabaseConnectionPool.h"

namespace
{
GroupResourceModel readResource(sql::ResultSet& result)
{
    GroupResourceModel item;
    item.setResourceId(result.getUInt64("resourceId"));
    item.setGroupId(result.getUInt64("groupId"));
    item.setResourceType(static_cast<std::uint8_t>(result.getUInt("resourceType")));
    item.setOriginalName(result.getString("originalName").asStdString());
    item.setStoredName(result.getString("storedName").asStdString());
    if (!result.isNull("coverStoredName"))
        item.setCoverStoredName(result.getString("coverStoredName").asStdString());
    item.setMimeType(result.getString("mimeType").asStdString());
    item.setFileSize(result.getUInt64("fileSize"));
    item.setUploaderId(result.getString("uploaderId").asStdString());
    item.setCreatedAt(result.getUInt64("createdAt"));
    return item;
}
}

std::uint64_t GroupResourceDao::insert(GroupResourceModel& resource) const
{
    auto connection = DatabaseConnectionPool::instance().acquire();
    std::unique_ptr<sql::PreparedStatement> statement(connection->prepareStatement(
        "INSERT INTO groupResource (groupId, resourceType, originalName, storedName, "
        "coverStoredName, mimeType, fileSize, uploaderId, createdAt) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    statement->setUInt64(1, resource.getGroupId());
    statement->setUInt(2, resource.getResourceType());
    statement->setString(3, resource.getOriginalName());
    statement->setString(4, resource.getStoredName());
    statement->setString(5, resource.getCoverStoredName());
    statement->setString(6, resource.getMimeType());
    statement->setUInt64(7, resource.getFileSize());
    statement->setString(8, resource.getUploaderId());
    statement->setUInt64(9, resource.getCreatedAt());
    statement->executeUpdate();
    std::unique_ptr<sql::PreparedStatement> idStatement(connection->prepareStatement("SELECT LAST_INSERT_ID() AS id"));
    std::unique_ptr<sql::ResultSet> result(idStatement->executeQuery());
    if (!result->next()) return 0;
    resource.setResourceId(result->getUInt64("id"));
    return resource.getResourceId();
}

std::vector<GroupResourceModel> GroupResourceDao::list(std::uint64_t groupId,
                                                       std::uint8_t resourceType) const
{
    auto connection = DatabaseConnectionPool::instance().acquire();
    std::unique_ptr<sql::PreparedStatement> statement(connection->prepareStatement(
        "SELECT resourceId, groupId, resourceType, originalName, storedName, coverStoredName, mimeType, "
        "fileSize, uploaderId, createdAt FROM groupResource "
        "WHERE groupId = ? AND resourceType = ? AND isDeleted = 0 ORDER BY createdAt DESC"));
    statement->setUInt64(1, groupId);
    statement->setUInt(2, resourceType);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    std::vector<GroupResourceModel> resources;
    while (result->next()) resources.push_back(readResource(*result));
    return resources;
}

std::optional<GroupResourceModel> GroupResourceDao::get(std::uint64_t resourceId) const
{
    auto connection = DatabaseConnectionPool::instance().acquire();
    std::unique_ptr<sql::PreparedStatement> statement(connection->prepareStatement(
        "SELECT resourceId, groupId, resourceType, originalName, storedName, coverStoredName, mimeType, "
        "fileSize, uploaderId, createdAt FROM groupResource "
        "WHERE resourceId = ? AND isDeleted = 0 LIMIT 1"));
    statement->setUInt64(1, resourceId);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    if (!result->next()) return std::nullopt;
    return readResource(*result);
}

bool GroupResourceDao::markDeleted(std::uint64_t resourceId,
                                   const std::string& deletedBy,
                                   std::uint64_t deletedAt) const
{
    auto connection = DatabaseConnectionPool::instance().acquire();
    std::unique_ptr<sql::PreparedStatement> statement(connection->prepareStatement(
        "UPDATE groupResource SET isDeleted = 1, deletedBy = ?, deletedAt = ? "
        "WHERE resourceId = ? AND isDeleted = 0"));
    statement->setString(1, deletedBy);
    statement->setUInt64(2, deletedAt);
    statement->setUInt64(3, resourceId);
    return statement->executeUpdate() > 0;
}
