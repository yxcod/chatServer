#include "UserSpaceDao.h"

#include <memory>
#include <stdexcept>

#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>

#include "DatabaseConnectionPool.h"

namespace
{
std::uint64_t lastInsertId(sql::Connection* connection)
{
    std::unique_ptr<sql::Statement> statement(connection->createStatement());
    std::unique_ptr<sql::ResultSet> result(
        statement->executeQuery("SELECT LAST_INSERT_ID() AS id"));
    if (!result->next()) throw std::runtime_error("Unable to read inserted id");
    return result->getUInt64("id");
}

SpaceGuestbookMessageModel readMessage(sql::ResultSet& result)
{
    SpaceGuestbookMessageModel message;
    message.setMessageId(result.getUInt64("messageId"));
    message.setOwnerUserName(result.getString("ownerUserName").asStdString());
    message.setAuthorUserName(result.getString("authorUserName").asStdString());
    message.setContent(result.getString("content").asStdString());
    message.setStatus(static_cast<std::uint8_t>(result.getUInt("status")));
    message.setCreatedAt(result.getUInt64("createdAt"));
    message.setUpdatedAt(result.getUInt64("updatedAt"));
    if (!result.isNull("deletedAt"))
        message.setDeletedAt(result.getUInt64("deletedAt"));
    message.setAuthorNickName(result.getString("authorNickName").asStdString());
    message.setAuthorAvatar(result.getString("authorAvatar").asStdString());
    return message;
}

SpaceGuestbookMessageModel getMessage(sql::Connection* connection,
                                      std::uint64_t messageId)
{
    std::unique_ptr<sql::PreparedStatement> statement(
        connection->prepareStatement(
            "SELECT m.messageId, m.ownerUserName, m.authorUserName, m.content, "
            "m.status, m.createdAt, m.updatedAt, m.deletedAt, "
            "COALESCE(NULLIF(u.nickName, ''), m.authorUserName) AS authorNickName, "
            "COALESCE(u.avatar, '') AS authorAvatar "
            "FROM spaceGuestbookMessage m LEFT JOIN userinfo u "
            "ON BINARY u.userName = BINARY m.authorUserName "
            "WHERE m.messageId = ? "
            "AND m.status = 0 LIMIT 1"));
    statement->setUInt64(1, messageId);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    if (!result->next()) throw std::runtime_error("Space message not found");
    return readMessage(*result);
}
}

UserSpaceModel UserSpaceDao::getSpace(const std::string& ownerUserName) const
{
    auto pooled = DatabaseConnectionPool::instance().acquire();
    std::unique_ptr<sql::PreparedStatement> statement(
        pooled->prepareStatement(
            "SELECT ownerUserName, coverImageUrl, createdAt, updatedAt "
            "FROM userSpace WHERE ownerUserName = ? LIMIT 1"));
    statement->setString(1, ownerUserName);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    UserSpaceModel space;
    space.setOwnerUserName(ownerUserName);
    if (!result->next()) return space;
    space.setCoverImageUrl(result->getString("coverImageUrl").asStdString());
    space.setCreatedAt(result->getUInt64("createdAt"));
    space.setUpdatedAt(result->getUInt64("updatedAt"));
    return space;
}

UserSpaceModel UserSpaceDao::updateCover(const std::string& ownerUserName,
                                         const std::string& coverImageUrl,
                                         std::uint64_t now) const
{
    auto pooled = DatabaseConnectionPool::instance().acquire();
    std::unique_ptr<sql::PreparedStatement> statement(
        pooled->prepareStatement(
            "INSERT INTO userSpace "
            "(ownerUserName, coverImageUrl, createdAt, updatedAt) "
            "VALUES (?, ?, ?, ?) ON DUPLICATE KEY UPDATE "
            "coverImageUrl = VALUES(coverImageUrl), updatedAt = VALUES(updatedAt)"));
    statement->setString(1, ownerUserName);
    statement->setString(2, coverImageUrl);
    statement->setUInt64(3, now);
    statement->setUInt64(4, now);
    statement->executeUpdate();
    UserSpaceModel space;
    space.setOwnerUserName(ownerUserName);
    space.setCoverImageUrl(coverImageUrl);
    space.setCreatedAt(now);
    space.setUpdatedAt(now);
    return space;
}

std::vector<SpaceGuestbookMessageModel> UserSpaceDao::listMessages(
    const std::string& ownerUserName,
    unsigned int limit) const
{
    auto pooled = DatabaseConnectionPool::instance().acquire();
    std::unique_ptr<sql::PreparedStatement> statement(
        pooled->prepareStatement(
            "SELECT m.messageId, m.ownerUserName, m.authorUserName, m.content, "
            "m.status, m.createdAt, m.updatedAt, m.deletedAt, "
            "COALESCE(NULLIF(u.nickName, ''), m.authorUserName) AS authorNickName, "
            "COALESCE(u.avatar, '') AS authorAvatar "
            "FROM spaceGuestbookMessage m LEFT JOIN userinfo u "
            "ON BINARY u.userName = BINARY m.authorUserName "
            "WHERE m.ownerUserName = ? "
            "AND m.status = 0 ORDER BY m.messageId DESC LIMIT ?"));
    statement->setString(1, ownerUserName);
    statement->setUInt(2, limit);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    std::vector<SpaceGuestbookMessageModel> messages;
    while (result->next()) messages.push_back(readMessage(*result));
    return messages;
}

SpaceGuestbookMessageModel UserSpaceDao::addMessage(
    const std::string& ownerUserName,
    const std::string& authorUserName,
    const std::string& content,
    std::uint64_t now) const
{
    auto pooled = DatabaseConnectionPool::instance().acquire();
    sql::Connection* connection = pooled.operator->();
    std::unique_ptr<sql::PreparedStatement> statement(
        connection->prepareStatement(
            "INSERT INTO spaceGuestbookMessage "
            "(ownerUserName, authorUserName, content, status, createdAt, updatedAt) "
            "VALUES (?, ?, ?, 0, ?, ?)"));
    statement->setString(1, ownerUserName);
    statement->setString(2, authorUserName);
    statement->setString(3, content);
    statement->setUInt64(4, now);
    statement->setUInt64(5, now);
    statement->executeUpdate();
    return getMessage(connection, lastInsertId(connection));
}

bool UserSpaceDao::deleteMessage(std::uint64_t messageId,
                                 const std::string& operatorUserName,
                                 std::uint64_t now) const
{
    auto pooled = DatabaseConnectionPool::instance().acquire();
    std::unique_ptr<sql::PreparedStatement> statement(
        pooled->prepareStatement(
            "UPDATE spaceGuestbookMessage SET status = 1, updatedAt = ?, "
            "deletedAt = ? WHERE messageId = ? AND status = 0 AND "
            "(ownerUserName = ? OR authorUserName = ?)"));
    statement->setUInt64(1, now);
    statement->setUInt64(2, now);
    statement->setUInt64(3, messageId);
    statement->setString(4, operatorUserName);
    statement->setString(5, operatorUserName);
    return statement->executeUpdate() == 1;
}
