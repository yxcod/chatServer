#include "MomentDao.h"

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
    if (!result->next())
    {
        throw std::runtime_error("Unable to read inserted id");
    }
    return result->getUInt64("id");
}

void rollbackQuietly(sql::Connection* connection)
{
    try
    {
        connection->rollback();
        connection->setAutoCommit(true);
    }
    catch (...)
    {
    }
}
}

Json::Value MomentDao::createMoment(const MomentCreateData& data) const
{
    auto pooled = DatabaseConnectionPool::instance().acquire();
    sql::Connection* connection = pooled.operator->();
    connection->setAutoCommit(false);

    try
    {
        if (!data.clientRequestId.empty())
        {
            std::unique_ptr<sql::PreparedStatement> existingStatement(
                connection->prepareStatement(
                    "SELECT momentId FROM moment "
                    "WHERE authorUserName = ? AND clientRequestId = ? LIMIT 1"));
            existingStatement->setString(1, data.authorUserName);
            existingStatement->setString(2, data.clientRequestId);
            std::unique_ptr<sql::ResultSet> existing(existingStatement->executeQuery());
            if (existing->next())
            {
                const auto momentId = existing->getUInt64("momentId");
                connection->commit();
                connection->setAutoCommit(true);
                return getMoment(connection, momentId, data.authorUserName);
            }
        }

        std::unique_ptr<sql::PreparedStatement> momentStatement(
            connection->prepareStatement(
                "INSERT INTO moment "
                "(authorUserName, content, visibility, locationName, likeCount, "
                "commentCount, clientRequestId, status, createdAt, updatedAt) "
                "VALUES (?, ?, ?, NULLIF(?, ''), 0, 0, NULLIF(?, ''), 0, ?, ?)"));
        momentStatement->setString(1, data.authorUserName);
        momentStatement->setString(2, data.content);
        momentStatement->setUInt(3, data.visibility);
        momentStatement->setString(4, data.locationName);
        momentStatement->setString(5, data.clientRequestId);
        momentStatement->setUInt64(6, data.createdAt);
        momentStatement->setUInt64(7, data.createdAt);
        momentStatement->executeUpdate();

        const auto momentId = lastInsertId(connection);
        if (!data.mediaUrls.empty())
        {
            std::unique_ptr<sql::PreparedStatement> mediaStatement(
                connection->prepareStatement(
                    "INSERT INTO momentMedia "
                    "(momentId, mediaType, mediaUrl, sortOrder, createdAt) "
                    "VALUES (?, 0, ?, ?, ?)"));
            for (std::size_t index = 0; index < data.mediaUrls.size(); ++index)
            {
                mediaStatement->setUInt64(1, momentId);
                mediaStatement->setString(2, data.mediaUrls[index]);
                mediaStatement->setUInt(3, static_cast<unsigned int>(index));
                mediaStatement->setUInt64(4, data.createdAt);
                mediaStatement->executeUpdate();
            }
        }

        connection->commit();
        connection->setAutoCommit(true);
        return getMoment(connection, momentId, data.authorUserName);
    }
    catch (...)
    {
        rollbackQuietly(connection);
        throw;
    }
}

Json::Value MomentDao::getOwnMoments(const std::string& userName,
                                     std::uint64_t beforeMomentId,
                                     unsigned int limit) const
{
    auto pooled = DatabaseConnectionPool::instance().acquire();
    sql::Connection* connection = pooled.operator->();
    const bool hasCursor = beforeMomentId > 0;
    const std::string query = hasCursor
        ? "SELECT momentId FROM moment WHERE authorUserName = ? AND status = 0 "
          "AND momentId < ? ORDER BY momentId DESC LIMIT ?"
        : "SELECT momentId FROM moment WHERE authorUserName = ? AND status = 0 "
          "ORDER BY momentId DESC LIMIT ?";

    std::unique_ptr<sql::PreparedStatement> statement(connection->prepareStatement(query));
    statement->setString(1, userName);
    if (hasCursor)
    {
        statement->setUInt64(2, beforeMomentId);
        statement->setUInt(3, limit);
    }
    else
    {
        statement->setUInt(2, limit);
    }

    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    std::vector<std::uint64_t> momentIds;
    while (result->next())
    {
        momentIds.push_back(result->getUInt64("momentId"));
    }

    Json::Value moments(Json::arrayValue);
    for (const auto momentId : momentIds)
    {
        Json::Value moment = getMoment(connection, momentId, userName);
        if (!moment.isNull()) moments.append(moment);
    }
    return moments;
}

Json::Value MomentDao::toggleLike(std::uint64_t momentId,
                                  const std::string& userName,
                                  std::uint64_t now) const
{
    auto pooled = DatabaseConnectionPool::instance().acquire();
    sql::Connection* connection = pooled.operator->();
    connection->setAutoCommit(false);

    try
    {
        std::unique_ptr<sql::PreparedStatement> lockStatement(
            connection->prepareStatement(
                "SELECT momentId FROM moment WHERE momentId = ? AND status = 0 FOR UPDATE"));
        lockStatement->setUInt64(1, momentId);
        std::unique_ptr<sql::ResultSet> locked(lockStatement->executeQuery());
        if (!locked->next()) throw std::runtime_error("Moment not found");

        std::unique_ptr<sql::PreparedStatement> findStatement(
            connection->prepareStatement(
                "SELECT likeId FROM momentLike WHERE momentId = ? AND userName = ? LIMIT 1"));
        findStatement->setUInt64(1, momentId);
        findStatement->setString(2, userName);
        std::unique_ptr<sql::ResultSet> found(findStatement->executeQuery());

        if (found->next())
        {
            std::unique_ptr<sql::PreparedStatement> deleteStatement(
                connection->prepareStatement(
                    "DELETE FROM momentLike WHERE momentId = ? AND userName = ?"));
            deleteStatement->setUInt64(1, momentId);
            deleteStatement->setString(2, userName);
            deleteStatement->executeUpdate();

            std::unique_ptr<sql::PreparedStatement> countStatement(
                connection->prepareStatement(
                    "UPDATE moment SET likeCount = GREATEST(likeCount - 1, 0), updatedAt = ? "
                    "WHERE momentId = ?"));
            countStatement->setUInt64(1, now);
            countStatement->setUInt64(2, momentId);
            countStatement->executeUpdate();
        }
        else
        {
            std::unique_ptr<sql::PreparedStatement> insertStatement(
                connection->prepareStatement(
                    "INSERT INTO momentLike (momentId, userName, createdAt) VALUES (?, ?, ?)"));
            insertStatement->setUInt64(1, momentId);
            insertStatement->setString(2, userName);
            insertStatement->setUInt64(3, now);
            insertStatement->executeUpdate();

            std::unique_ptr<sql::PreparedStatement> countStatement(
                connection->prepareStatement(
                    "UPDATE moment SET likeCount = likeCount + 1, updatedAt = ? WHERE momentId = ?"));
            countStatement->setUInt64(1, now);
            countStatement->setUInt64(2, momentId);
            countStatement->executeUpdate();
        }

        connection->commit();
        connection->setAutoCommit(true);
        return getMoment(connection, momentId, userName);
    }
    catch (...)
    {
        rollbackQuietly(connection);
        throw;
    }
}

Json::Value MomentDao::addComment(std::uint64_t momentId,
                                  const std::string& userName,
                                  const std::string& content,
                                  std::uint64_t now) const
{
    auto pooled = DatabaseConnectionPool::instance().acquire();
    sql::Connection* connection = pooled.operator->();
    connection->setAutoCommit(false);

    try
    {
        std::unique_ptr<sql::PreparedStatement> lockStatement(
            connection->prepareStatement(
                "SELECT momentId FROM moment WHERE momentId = ? AND status = 0 FOR UPDATE"));
        lockStatement->setUInt64(1, momentId);
        std::unique_ptr<sql::ResultSet> locked(lockStatement->executeQuery());
        if (!locked->next()) throw std::runtime_error("Moment not found");

        std::unique_ptr<sql::PreparedStatement> commentStatement(
            connection->prepareStatement(
                "INSERT INTO momentComment "
                "(momentId, userName, content, status, createdAt, updatedAt) "
                "VALUES (?, ?, ?, 0, ?, ?)"));
        commentStatement->setUInt64(1, momentId);
        commentStatement->setString(2, userName);
        commentStatement->setString(3, content);
        commentStatement->setUInt64(4, now);
        commentStatement->setUInt64(5, now);
        commentStatement->executeUpdate();

        std::unique_ptr<sql::PreparedStatement> countStatement(
            connection->prepareStatement(
                "UPDATE moment SET commentCount = commentCount + 1, updatedAt = ? WHERE momentId = ?"));
        countStatement->setUInt64(1, now);
        countStatement->setUInt64(2, momentId);
        countStatement->executeUpdate();

        connection->commit();
        connection->setAutoCommit(true);
        return getMoment(connection, momentId, userName);
    }
    catch (...)
    {
        rollbackQuietly(connection);
        throw;
    }
}

Json::Value MomentDao::getMoment(sql::Connection* connection,
                                 std::uint64_t momentId,
                                 const std::string& viewerUserName) const
{
    std::unique_ptr<sql::PreparedStatement> statement(
        connection->prepareStatement(
            "SELECT m.momentId, m.authorUserName, m.content, m.visibility, "
            "m.locationName, m.likeCount, m.commentCount, m.createdAt, "
            "u.nickName, u.avatar, "
            "EXISTS(SELECT 1 FROM momentLike ml WHERE ml.momentId = m.momentId "
            "AND ml.userName = ?) AS isLiked "
            "FROM moment m JOIN userinfo u ON u.userName = m.authorUserName "
            "WHERE m.momentId = ? AND m.status = 0 LIMIT 1"));
    statement->setString(1, viewerUserName);
    statement->setUInt64(2, momentId);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    if (!result->next()) return Json::Value();

    Json::Value moment(Json::objectValue);
    moment["id"] = Json::UInt64(result->getUInt64("momentId"));
    moment["authorId"] = result->getString("authorUserName").asStdString();
    moment["authorName"] = result->getString("nickName").asStdString();
    moment["authorAvatarUrl"] = result->getString("avatar").asStdString();
    moment["content"] = result->getString("content").asStdString();
    moment["visibility"] = result->getUInt("visibility");
    const std::string locationName = result->getString("locationName");
    moment["location"] = locationName.empty() ? Json::Value() : Json::Value(locationName);
    moment["likeCount"] = result->getUInt("likeCount");
    moment["commentCount"] = result->getUInt("commentCount");
    moment["isLiked"] = result->getBoolean("isLiked");
    moment["createdAt"] = Json::UInt64(result->getUInt64("createdAt"));
    appendMedia(connection, momentId, moment);
    appendComments(connection, momentId, moment);
    return moment;
}

void MomentDao::appendMedia(sql::Connection* connection,
                            std::uint64_t momentId,
                            Json::Value& moment) const
{
    std::unique_ptr<sql::PreparedStatement> statement(
        connection->prepareStatement(
            "SELECT mediaUrl FROM momentMedia WHERE momentId = ? ORDER BY sortOrder ASC"));
    statement->setUInt64(1, momentId);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    Json::Value media(Json::arrayValue);
    while (result->next())
    {
        media.append(result->getString("mediaUrl").asStdString());
    }
    moment["mediaPaths"] = media;
}

void MomentDao::appendComments(sql::Connection* connection,
                               std::uint64_t momentId,
                               Json::Value& moment) const
{
    std::unique_ptr<sql::PreparedStatement> statement(
        connection->prepareStatement(
            "SELECT c.commentId, c.userName, c.content, c.createdAt, u.nickName "
            "FROM momentComment c JOIN userinfo u ON u.userName = c.userName "
            "WHERE c.momentId = ? AND c.status = 0 "
            "ORDER BY c.createdAt ASC, c.commentId ASC"));
    statement->setUInt64(1, momentId);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    Json::Value comments(Json::arrayValue);
    while (result->next())
    {
        Json::Value comment(Json::objectValue);
        comment["id"] = Json::UInt64(result->getUInt64("commentId"));
        comment["userId"] = result->getString("userName").asStdString();
        comment["displayName"] = result->getString("nickName").asStdString();
        comment["content"] = result->getString("content").asStdString();
        comment["createdAt"] = Json::UInt64(result->getUInt64("createdAt"));
        comments.append(comment);
    }
    moment["comments"] = comments;
}
