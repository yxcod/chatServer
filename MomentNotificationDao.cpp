#include "MomentNotificationDao.h"

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
    if (!result->next()) throw std::runtime_error("Unable to read notification id");
    return result->getUInt64("id");
}

MomentNotificationModel readNotification(sql::ResultSet& result)
{
    MomentNotificationModel item;
    item.setNotificationId(result.getUInt64("notificationId"));
    item.setRecipientUserName(
        result.getString("recipientUserName").asStdString());
    item.setActorUserName(result.getString("actorUserName").asStdString());
    item.setMomentId(result.getUInt64("momentId"));
    item.setInteractionType(static_cast<std::uint8_t>(
        result.getUInt("interactionType")));
    item.setCommentContent(result.getString("commentContent").asStdString());
    item.setRead(result.getBoolean("isRead"));
    item.setCreatedAt(result.getUInt64("createdAt"));
    if (!result.isNull("readAt")) item.setReadAt(result.getUInt64("readAt"));
    item.setActorNickName(result.getString("actorNickName").asStdString());
    item.setActorAvatar(result.getString("actorAvatar").asStdString());
    return item;
}

MomentNotificationModel getNotification(sql::Connection* connection,
                                        std::uint64_t notificationId)
{
    std::unique_ptr<sql::PreparedStatement> statement(
        connection->prepareStatement(
            "SELECT n.notificationId, n.recipientUserName, n.actorUserName, "
            "n.momentId, n.interactionType, n.commentContent, n.isRead, "
            "n.createdAt, n.readAt, "
            "COALESCE(NULLIF(u.nickName, ''), n.actorUserName) AS actorNickName, "
            "COALESCE(u.avatar, '') AS actorAvatar "
            "FROM momentNotification n LEFT JOIN userinfo u "
            "ON BINARY u.userName = BINARY n.actorUserName "
            "WHERE n.notificationId = ? LIMIT 1"));
    statement->setUInt64(1, notificationId);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    if (!result->next()) throw std::runtime_error("Moment notification not found");
    return readNotification(*result);
}
}

MomentNotificationModel MomentNotificationDao::create(
    const std::string& recipientUserName,
    const std::string& actorUserName,
    std::uint64_t momentId,
    std::uint8_t interactionType,
    const std::string& commentContent,
    std::uint64_t createdAt) const
{
    auto pooled = DatabaseConnectionPool::instance().acquire();
    sql::Connection* connection = pooled.operator->();
    std::unique_ptr<sql::PreparedStatement> statement(
        connection->prepareStatement(
            "INSERT INTO momentNotification "
            "(recipientUserName, actorUserName, momentId, interactionType, "
            "commentContent, isRead, createdAt, readAt) "
            "VALUES (?, ?, ?, ?, ?, 0, ?, NULL)"));
    statement->setString(1, recipientUserName);
    statement->setString(2, actorUserName);
    statement->setUInt64(3, momentId);
    statement->setUInt(4, interactionType);
    statement->setString(5, commentContent);
    statement->setUInt64(6, createdAt);
    statement->executeUpdate();
    return getNotification(connection, lastInsertId(connection));
}

std::vector<MomentNotificationModel> MomentNotificationDao::list(
    const std::string& recipientUserName,
    std::uint64_t beforeNotificationId,
    unsigned int limit) const
{
    auto pooled = DatabaseConnectionPool::instance().acquire();
    const bool hasCursor = beforeNotificationId > 0;
    const std::string query =
        "SELECT n.notificationId, n.recipientUserName, n.actorUserName, "
        "n.momentId, n.interactionType, n.commentContent, n.isRead, "
        "n.createdAt, n.readAt, "
        "COALESCE(NULLIF(u.nickName, ''), n.actorUserName) AS actorNickName, "
        "COALESCE(u.avatar, '') AS actorAvatar "
        "FROM momentNotification n LEFT JOIN userinfo u "
        "ON BINARY u.userName = BINARY n.actorUserName "
        "WHERE n.recipientUserName = ? " +
        std::string(hasCursor ? "AND n.notificationId < ? " : "") +
        "ORDER BY n.notificationId DESC LIMIT ?";
    std::unique_ptr<sql::PreparedStatement> statement(
        pooled->prepareStatement(query));
    statement->setString(1, recipientUserName);
    if (hasCursor)
    {
        statement->setUInt64(2, beforeNotificationId);
        statement->setUInt(3, limit);
    }
    else
    {
        statement->setUInt(2, limit);
    }
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    std::vector<MomentNotificationModel> notifications;
    while (result->next()) notifications.push_back(readNotification(*result));
    return notifications;
}

unsigned int MomentNotificationDao::unreadCount(
    const std::string& recipientUserName) const
{
    auto pooled = DatabaseConnectionPool::instance().acquire();
    std::unique_ptr<sql::PreparedStatement> statement(
        pooled->prepareStatement(
            "SELECT COUNT(*) AS count FROM momentNotification "
            "WHERE recipientUserName = ? AND isRead = 0"));
    statement->setString(1, recipientUserName);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    return result->next() ? result->getUInt("count") : 0U;
}

unsigned int MomentNotificationDao::markAllRead(
    const std::string& recipientUserName,
    std::uint64_t readAt) const
{
    auto pooled = DatabaseConnectionPool::instance().acquire();
    std::unique_ptr<sql::PreparedStatement> statement(
        pooled->prepareStatement(
            "UPDATE momentNotification SET isRead = 1, readAt = ? "
            "WHERE recipientUserName = ? AND isRead = 0"));
    statement->setUInt64(1, readAt);
    statement->setString(2, recipientUserName);
    return static_cast<unsigned int>(statement->executeUpdate());
}
