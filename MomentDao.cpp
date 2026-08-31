#include "MomentDao.h"

#include <memory>
#include <stdexcept>

#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>

#include "DatabaseConnectionPool.h"
#include "MomentLikeModel.h"

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

MomentModel MomentDao::createMoment(
    MomentModel moment,
    const std::vector<MomentMediaModel>& media) const
{
    auto pooled = DatabaseConnectionPool::instance().acquire();
    sql::Connection* connection = pooled.operator->();
    connection->setAutoCommit(false);

    try
    {
        if (!moment.getClientRequestId().empty())
        {
            std::unique_ptr<sql::PreparedStatement> existingStatement(
                connection->prepareStatement(
                    "SELECT momentId FROM moment "
                    "WHERE authorUserName = ? AND clientRequestId = ? LIMIT 1"));
            existingStatement->setString(1, moment.getAuthorUserName());
            existingStatement->setString(2, moment.getClientRequestId());
            std::unique_ptr<sql::ResultSet> existing(existingStatement->executeQuery());
            if (existing->next())
            {
                const auto momentId = existing->getUInt64("momentId");
                connection->commit();
                connection->setAutoCommit(true);
                return getMoment(connection, momentId, moment.getAuthorUserName());
            }
        }

        std::unique_ptr<sql::PreparedStatement> momentStatement(
            connection->prepareStatement(
                "INSERT INTO moment "
                "(authorUserName, content, visibility, locationName, likeCount, "
                "commentCount, clientRequestId, status, createdAt, updatedAt) "
                "VALUES (?, ?, ?, NULLIF(?, ''), 0, 0, NULLIF(?, ''), 0, ?, ?)"));
        momentStatement->setString(1, moment.getAuthorUserName());
        momentStatement->setString(2, moment.getContent());
        momentStatement->setUInt(3, moment.getVisibility());
        momentStatement->setString(4, moment.getLocationName());
        momentStatement->setString(5, moment.getClientRequestId());
        momentStatement->setUInt64(6, moment.getCreatedAt());
        momentStatement->setUInt64(7, moment.getUpdatedAt());
        momentStatement->executeUpdate();

        const auto momentId = lastInsertId(connection);
        if (!media.empty())
        {
            std::unique_ptr<sql::PreparedStatement> mediaStatement(
                connection->prepareStatement(
                    "INSERT INTO momentMedia "
                    "(momentId, mediaType, mediaUrl, sortOrder, createdAt) "
                    "VALUES (?, ?, ?, ?, ?)"));
            for (const auto& mediaItem : media)
            {
                mediaStatement->setUInt64(1, momentId);
                mediaStatement->setUInt(2, mediaItem.getMediaType());
                mediaStatement->setString(3, mediaItem.getMediaUrl());
                mediaStatement->setUInt(4, mediaItem.getSortOrder());
                mediaStatement->setUInt64(5, mediaItem.getCreatedAt());
                mediaStatement->executeUpdate();
            }
        }

        connection->commit();
        connection->setAutoCommit(true);
        return getMoment(connection, momentId, moment.getAuthorUserName());
    }
    catch (...)
    {
        rollbackQuietly(connection);
        throw;
    }
}

std::vector<MomentModel> MomentDao::getOwnMoments(
    const std::string& userName,
    std::uint64_t beforeMomentId,
    unsigned int limit) const
{
    return getVisibleMoments(userName, userName, beforeMomentId, limit);
}

std::vector<MomentModel> MomentDao::getVisibleMoments(
    const std::string& viewerUserName,
    const std::string& authorUserName,
    std::uint64_t beforeMomentId,
    unsigned int limit) const
{
    auto pooled = DatabaseConnectionPool::instance().acquire();
    sql::Connection* connection = pooled.operator->();
    const bool hasCursor = beforeMomentId > 0;
    const std::string query = hasCursor
        ? "SELECT m.momentId FROM moment m "
          "WHERE m.authorUserName = ? AND m.status = 0 AND m.momentId < ? "
          "AND (m.authorUserName = ? OR m.visibility = 0 OR "
          "(m.visibility = 1 AND EXISTS (SELECT 1 FROM friendrelation fr "
          "WHERE fr.status = 1 AND ((fr.fromUserId = ? AND fr.toUserId = m.authorUserName) "
          "OR (fr.toUserId = ? AND fr.fromUserId = m.authorUserName))))) "
          "ORDER BY m.momentId DESC LIMIT ?"
        : "SELECT m.momentId FROM moment m "
          "WHERE m.authorUserName = ? AND m.status = 0 "
          "AND (m.authorUserName = ? OR m.visibility = 0 OR "
          "(m.visibility = 1 AND EXISTS (SELECT 1 FROM friendrelation fr "
          "WHERE fr.status = 1 AND ((fr.fromUserId = ? AND fr.toUserId = m.authorUserName) "
          "OR (fr.toUserId = ? AND fr.fromUserId = m.authorUserName))))) "
          "ORDER BY m.momentId DESC LIMIT ?";

    std::unique_ptr<sql::PreparedStatement> statement(connection->prepareStatement(query));
    statement->setString(1, authorUserName);
    if (hasCursor)
    {
        statement->setUInt64(2, beforeMomentId);
        statement->setString(3, viewerUserName);
        statement->setString(4, viewerUserName);
        statement->setString(5, viewerUserName);
        statement->setUInt(6, limit);
    }
    else
    {
        statement->setString(2, viewerUserName);
        statement->setString(3, viewerUserName);
        statement->setString(4, viewerUserName);
        statement->setUInt(5, limit);
    }

    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    std::vector<std::uint64_t> momentIds;
    while (result->next())
    {
        momentIds.push_back(result->getUInt64("momentId"));
    }

    std::vector<MomentModel> moments;
    moments.reserve(momentIds.size());
    for (const auto momentId : momentIds)
    {
        moments.push_back(getMoment(connection, momentId, viewerUserName));
    }
    return moments;
}

MomentModel MomentDao::toggleLike(std::uint64_t momentId,
                                  const std::string& userName,
                                  std::uint64_t now) const
{
    auto pooled = DatabaseConnectionPool::instance().acquire();
    sql::Connection* connection = pooled.operator->();
    connection->setAutoCommit(false);

    try
    {
        lockVisibleMoment(connection, momentId, userName);

        std::unique_ptr<sql::PreparedStatement> findStatement(
            connection->prepareStatement(
                "SELECT likeId FROM momentLike WHERE momentId = ? AND userName = ? LIMIT 1"));
        findStatement->setUInt64(1, momentId);
        findStatement->setString(2, userName);
        std::unique_ptr<sql::ResultSet> found(findStatement->executeQuery());

        MomentLikeModel like;
        like.setMomentId(momentId);
        like.setUserName(userName);
        like.setCreatedAt(now);
        if (found->next())
        {
            like.setLikeId(found->getUInt64("likeId"));
            std::unique_ptr<sql::PreparedStatement> deleteStatement(
                connection->prepareStatement(
                    "DELETE FROM momentLike WHERE momentId = ? AND userName = ?"));
            deleteStatement->setUInt64(1, like.getMomentId());
            deleteStatement->setString(2, like.getUserName());
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
            insertStatement->setUInt64(1, like.getMomentId());
            insertStatement->setString(2, like.getUserName());
            insertStatement->setUInt64(3, like.getCreatedAt());
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

MomentModel MomentDao::addComment(std::uint64_t momentId,
                                  const std::string& userName,
                                  const std::string& content,
                                  std::uint64_t now) const
{
    auto pooled = DatabaseConnectionPool::instance().acquire();
    sql::Connection* connection = pooled.operator->();
    connection->setAutoCommit(false);

    try
    {
        lockVisibleMoment(connection, momentId, userName);

        MomentCommentModel comment;
        comment.setMomentId(momentId);
        comment.setUserName(userName);
        comment.setContent(content);
        comment.setCreatedAt(now);
        comment.setUpdatedAt(now);

        std::unique_ptr<sql::PreparedStatement> commentStatement(
            connection->prepareStatement(
                "INSERT INTO momentComment "
                "(momentId, userName, content, status, createdAt, updatedAt) "
                "VALUES (?, ?, ?, 0, ?, ?)"));
        commentStatement->setUInt64(1, comment.getMomentId());
        commentStatement->setString(2, comment.getUserName());
        commentStatement->setString(3, comment.getContent());
        commentStatement->setUInt64(4, comment.getCreatedAt());
        commentStatement->setUInt64(5, comment.getUpdatedAt());
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

std::vector<std::string> MomentDao::deleteMoment(
    std::uint64_t momentId,
    const std::string& authorUserName) const
{
    auto pooled = DatabaseConnectionPool::instance().acquire();
    sql::Connection* connection = pooled.operator->();
    connection->setAutoCommit(false);

    try
    {
        std::unique_ptr<sql::PreparedStatement> ownerStatement(
            connection->prepareStatement(
                "SELECT momentId FROM moment "
                "WHERE momentId = ? AND authorUserName = ? FOR UPDATE"));
        ownerStatement->setUInt64(1, momentId);
        ownerStatement->setString(2, authorUserName);
        std::unique_ptr<sql::ResultSet> owner(ownerStatement->executeQuery());
        if (!owner->next())
        {
            throw std::runtime_error("Moment not found or not owned by user");
        }

        std::vector<std::string> mediaUrls;
        std::unique_ptr<sql::PreparedStatement> mediaStatement(
            connection->prepareStatement(
                "SELECT mediaUrl FROM momentMedia WHERE momentId = ?"));
        mediaStatement->setUInt64(1, momentId);
        std::unique_ptr<sql::ResultSet> media(mediaStatement->executeQuery());
        while (media->next())
        {
            mediaUrls.push_back(media->getString("mediaUrl").asStdString());
        }

        // Break possible reply-to-comment references before removing all
        // comments belonging to the moment.
        std::unique_ptr<sql::PreparedStatement> clearReplies(
            connection->prepareStatement(
                "UPDATE momentComment SET replyToCommentId = NULL, "
                "replyToUserName = NULL WHERE momentId = ?"));
        clearReplies->setUInt64(1, momentId);
        clearReplies->executeUpdate();

        const char* childTables[] = {"momentLike", "momentComment", "momentMedia"};
        for (const char* table : childTables)
        {
            std::unique_ptr<sql::PreparedStatement> statement(
                connection->prepareStatement(
                    std::string("DELETE FROM ") + table + " WHERE momentId = ?"));
            statement->setUInt64(1, momentId);
            statement->executeUpdate();
        }

        std::unique_ptr<sql::PreparedStatement> momentStatement(
            connection->prepareStatement(
                "DELETE FROM moment WHERE momentId = ? AND authorUserName = ?"));
        momentStatement->setUInt64(1, momentId);
        momentStatement->setString(2, authorUserName);
        if (momentStatement->executeUpdate() != 1)
        {
            throw std::runtime_error("Unable to delete moment");
        }

        connection->commit();
        connection->setAutoCommit(true);
        return mediaUrls;
    }
    catch (...)
    {
        rollbackQuietly(connection);
        throw;
    }
}

MomentModel MomentDao::getMoment(sql::Connection* connection,
                                 std::uint64_t momentId,
                                 const std::string& viewerUserName) const
{
    std::unique_ptr<sql::PreparedStatement> statement(
        connection->prepareStatement(
            "SELECT m.momentId, m.authorUserName, m.content, m.visibility, "
            "m.locationName, m.latitude, m.longitude, m.likeCount, m.commentCount, "
            "m.clientRequestId, m.status, m.createdAt, m.updatedAt, m.deletedAt, "
            "u.nickName, u.avatar, "
            "EXISTS(SELECT 1 FROM momentLike ml WHERE ml.momentId = m.momentId "
            "AND ml.userName = ?) AS isLiked "
            "FROM moment m JOIN userinfo u ON u.userName = m.authorUserName "
            "WHERE m.momentId = ? AND m.status = 0 "
            "AND (m.authorUserName = ? OR m.visibility = 0 OR "
            "(m.visibility = 1 AND EXISTS (SELECT 1 FROM friendrelation fr "
            "WHERE fr.status = 1 AND ((fr.fromUserId = ? AND fr.toUserId = m.authorUserName) "
            "OR (fr.toUserId = ? AND fr.fromUserId = m.authorUserName))))) LIMIT 1"));
    statement->setString(1, viewerUserName);
    statement->setUInt64(2, momentId);
    statement->setString(3, viewerUserName);
    statement->setString(4, viewerUserName);
    statement->setString(5, viewerUserName);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    if (!result->next()) throw std::runtime_error("Moment not found");

    MomentModel moment;
    moment.setMomentId(result->getUInt64("momentId"));
    moment.setAuthorUserName(result->getString("authorUserName").asStdString());
    moment.setAuthorNickName(result->getString("nickName").asStdString());
    moment.setAuthorAvatar(result->getString("avatar").asStdString());
    moment.setContent(result->getString("content").asStdString());
    moment.setVisibility(static_cast<std::uint8_t>(result->getUInt("visibility")));
    const std::string locationName = result->getString("locationName");
    moment.setLocationName(locationName);
    if (!result->isNull("latitude"))
    {
        moment.setLatitude(static_cast<double>(result->getDouble("latitude")));
    }
    if (!result->isNull("longitude"))
    {
        moment.setLongitude(static_cast<double>(result->getDouble("longitude")));
    }
    moment.setLikeCount(result->getUInt("likeCount"));
    moment.setCommentCount(result->getUInt("commentCount"));
    if (!result->isNull("clientRequestId"))
    {
        moment.setClientRequestId(result->getString("clientRequestId").asStdString());
    }
    moment.setStatus(static_cast<std::uint8_t>(result->getUInt("status")));
    moment.setLikedByViewer(result->getBoolean("isLiked"));
    moment.setCreatedAt(result->getUInt64("createdAt"));
    moment.setUpdatedAt(result->getUInt64("updatedAt"));
    if (!result->isNull("deletedAt"))
    {
        moment.setDeletedAt(result->getUInt64("deletedAt"));
    }
    moment.setMedia(getMedia(connection, momentId));
    moment.setComments(getComments(connection, momentId));
    return moment;
}

std::vector<MomentMediaModel> MomentDao::getMedia(
    sql::Connection* connection,
    std::uint64_t momentId) const
{
    std::unique_ptr<sql::PreparedStatement> statement(
        connection->prepareStatement(
            "SELECT mediaId, momentId, mediaType, mediaUrl, thumbnailUrl, width, height, "
            "fileSize, fileHash, sortOrder, createdAt FROM momentMedia "
            "WHERE momentId = ? ORDER BY sortOrder ASC"));
    statement->setUInt64(1, momentId);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    std::vector<MomentMediaModel> media;
    while (result->next())
    {
        MomentMediaModel item;
        item.setMediaId(result->getUInt64("mediaId"));
        item.setMomentId(result->getUInt64("momentId"));
        item.setMediaType(static_cast<std::uint8_t>(result->getUInt("mediaType")));
        item.setMediaUrl(result->getString("mediaUrl").asStdString());
        if (!result->isNull("thumbnailUrl"))
        {
            item.setThumbnailUrl(result->getString("thumbnailUrl").asStdString());
        }
        if (!result->isNull("width")) item.setWidth(result->getUInt("width"));
        if (!result->isNull("height")) item.setHeight(result->getUInt("height"));
        if (!result->isNull("fileSize")) item.setFileSize(result->getUInt64("fileSize"));
        if (!result->isNull("fileHash"))
        {
            item.setFileHash(result->getString("fileHash").asStdString());
        }
        item.setSortOrder(static_cast<std::uint16_t>(result->getUInt("sortOrder")));
        item.setCreatedAt(result->getUInt64("createdAt"));
        media.push_back(std::move(item));
    }
    return media;
}

std::vector<MomentCommentModel> MomentDao::getComments(
    sql::Connection* connection,
    std::uint64_t momentId) const
{
    std::unique_ptr<sql::PreparedStatement> statement(
        connection->prepareStatement(
            "SELECT c.commentId, c.momentId, c.userName, c.replyToCommentId, "
            "c.replyToUserName, c.content, c.status, c.createdAt, c.updatedAt, "
            "c.deletedAt, u.nickName "
            "FROM momentComment c JOIN userinfo u ON u.userName = c.userName "
            "WHERE c.momentId = ? AND c.status = 0 "
            "ORDER BY c.createdAt ASC, c.commentId ASC"));
    statement->setUInt64(1, momentId);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    std::vector<MomentCommentModel> comments;
    while (result->next())
    {
        MomentCommentModel comment;
        comment.setCommentId(result->getUInt64("commentId"));
        comment.setMomentId(result->getUInt64("momentId"));
        comment.setUserName(result->getString("userName").asStdString());
        if (!result->isNull("replyToCommentId"))
        {
            comment.setReplyToCommentId(result->getUInt64("replyToCommentId"));
        }
        if (!result->isNull("replyToUserName"))
        {
            comment.setReplyToUserName(result->getString("replyToUserName").asStdString());
        }
        comment.setDisplayName(result->getString("nickName").asStdString());
        comment.setContent(result->getString("content").asStdString());
        comment.setStatus(static_cast<std::uint8_t>(result->getUInt("status")));
        comment.setCreatedAt(result->getUInt64("createdAt"));
        comment.setUpdatedAt(result->getUInt64("updatedAt"));
        if (!result->isNull("deletedAt"))
        {
            comment.setDeletedAt(result->getUInt64("deletedAt"));
        }
        comments.push_back(std::move(comment));
    }
    return comments;
}

void MomentDao::lockVisibleMoment(
    sql::Connection* connection,
    std::uint64_t momentId,
    const std::string& viewerUserName) const
{
    std::unique_ptr<sql::PreparedStatement> statement(
        connection->prepareStatement(
            "SELECT m.momentId FROM moment m WHERE m.momentId = ? AND m.status = 0 "
            "AND (m.authorUserName = ? OR m.visibility = 0 OR "
            "(m.visibility = 1 AND EXISTS (SELECT 1 FROM friendrelation fr "
            "WHERE fr.status = 1 AND ((fr.fromUserId = ? AND fr.toUserId = m.authorUserName) "
            "OR (fr.toUserId = ? AND fr.fromUserId = m.authorUserName))))) FOR UPDATE"));
    statement->setUInt64(1, momentId);
    statement->setString(2, viewerUserName);
    statement->setString(3, viewerUserName);
    statement->setString(4, viewerUserName);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    if (!result->next())
    {
        throw std::runtime_error("Moment not found or not visible");
    }
}

std::string MomentDao::getAuthorUserName(std::uint64_t momentId) const
{
    auto connection = DatabaseConnectionPool::instance().acquire();
    std::unique_ptr<sql::PreparedStatement> statement(
        connection->prepareStatement(
            "SELECT authorUserName FROM moment WHERE momentId = ? AND status = 0 LIMIT 1"));
    statement->setUInt64(1, momentId);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    return result->next()
        ? result->getString("authorUserName").asStdString()
        : std::string();
}
