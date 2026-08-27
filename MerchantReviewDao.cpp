#include "MerchantReviewDao.h"

#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>

#include "DatabaseConnectionPool.h"
#include "MerchantReviewReactionModel.h"

namespace
{
constexpr double kMissingNumber = -999.0;

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

std::uint64_t lastInsertId(sql::Connection* connection)
{
    std::unique_ptr<sql::Statement> statement(connection->createStatement());
    std::unique_ptr<sql::ResultSet> result(
        statement->executeQuery("SELECT LAST_INSERT_ID() AS id"));
    if (!result->next()) throw std::runtime_error("Unable to read inserted id");
    return result->getUInt64("id");
}

MerchantReviewEntryModel readEntry(sql::ResultSet& result)
{
    MerchantReviewEntryModel entry;
    entry.setEntryId(result.getUInt64("entryId"));
    entry.setOwnerUserName(result.getString("ownerUserName").asStdString());
    entry.setPoiId(result.getString("poiId").asStdString());
    entry.setMerchantName(result.getString("merchantName").asStdString());
    entry.setAddress(result.getString("address").asStdString());
    entry.setCategory(result.getString("category").asStdString());
    if (!result.isNull("distanceMeters"))
        entry.setDistanceMeters(result.getUInt("distanceMeters"));
    if (!result.isNull("rating")) entry.setRating(result.getDouble("rating"));
    entry.setImageUrl(result.getString("imageUrl").asStdString());
    if (!result.isNull("imageUrlsJson"))
        entry.setImageUrlsJson(result.getString("imageUrlsJson").asStdString());
    entry.setPhone(result.getString("phone").asStdString());
    entry.setOpeningHours(result.getString("openingHours").asStdString());
    if (!result.isNull("price")) entry.setPrice(result.getDouble("price"));
    entry.setDetailUrl(result.getString("detailUrl").asStdString());
    entry.setImageCount(result.getUInt("imageCount"));
    if (!result.isNull("latitude")) entry.setLatitude(result.getDouble("latitude"));
    if (!result.isNull("longitude")) entry.setLongitude(result.getDouble("longitude"));
    entry.setLikeCount(result.getUInt("likeCount"));
    entry.setDislikeCount(result.getUInt("dislikeCount"));
    entry.setCommentCount(result.getUInt("commentCount"));
    entry.setStatus(static_cast<std::uint8_t>(result.getUInt("status")));
    entry.setCreatedAt(result.getUInt64("createdAt"));
    entry.setUpdatedAt(result.getUInt64("updatedAt"));
    entry.setViewerReaction(
        static_cast<std::uint8_t>(result.getUInt("viewerReaction")));
    return entry;
}
}

MerchantReviewEntryModel MerchantReviewDao::addEntry(
    MerchantReviewEntryModel entry) const
{
    auto pooled = DatabaseConnectionPool::instance().acquire();
    sql::Connection* connection = pooled.operator->();
    std::unique_ptr<sql::PreparedStatement> statement(
        connection->prepareStatement(
            "INSERT INTO merchantReviewEntry "
            "(ownerUserName, poiId, merchantName, address, category, distanceMeters, "
            "rating, imageUrl, imageUrlsJson, phone, openingHours, price, detailUrl, "
            "imageCount, latitude, longitude, likeCount, dislikeCount, commentCount, "
            "status, createdAt, updatedAt) VALUES "
            "(?, ?, ?, ?, ?, NULLIF(?, 4294967295), NULLIF(?, -999.0), ?, "
            "NULLIF(?, ''), ?, ?, NULLIF(?, -999.0), ?, ?, NULLIF(?, -999.0), "
            "NULLIF(?, -999.0), 0, 0, 0, 0, ?, ?) "
            "ON DUPLICATE KEY UPDATE entryId = LAST_INSERT_ID(entryId), "
            "merchantName = VALUES(merchantName), address = VALUES(address), "
            "category = VALUES(category), distanceMeters = VALUES(distanceMeters), "
            "rating = VALUES(rating), imageUrl = VALUES(imageUrl), "
            "imageUrlsJson = VALUES(imageUrlsJson), phone = VALUES(phone), "
            "openingHours = VALUES(openingHours), price = VALUES(price), "
            "detailUrl = VALUES(detailUrl), imageCount = VALUES(imageCount), "
            "latitude = VALUES(latitude), longitude = VALUES(longitude), "
            "updatedAt = VALUES(updatedAt)"));
    statement->setString(1, entry.getOwnerUserName());
    statement->setString(2, entry.getPoiId());
    statement->setString(3, entry.getMerchantName());
    statement->setString(4, entry.getAddress());
    statement->setString(5, entry.getCategory());
    statement->setUInt(
        6,
        entry.hasDistanceMeters()
            ? entry.getDistanceMeters()
            : std::numeric_limits<std::uint32_t>::max());
    statement->setDouble(7, entry.hasRating() ? entry.getRating() : kMissingNumber);
    statement->setString(8, entry.getImageUrl());
    statement->setString(9, entry.getImageUrlsJson());
    statement->setString(10, entry.getPhone());
    statement->setString(11, entry.getOpeningHours());
    statement->setDouble(12, entry.hasPrice() ? entry.getPrice() : kMissingNumber);
    statement->setString(13, entry.getDetailUrl());
    statement->setUInt(14, entry.getImageCount());
    statement->setDouble(15, entry.hasLatitude() ? entry.getLatitude() : kMissingNumber);
    statement->setDouble(16, entry.hasLongitude() ? entry.getLongitude() : kMissingNumber);
    statement->setUInt64(17, entry.getCreatedAt());
    statement->setUInt64(18, entry.getUpdatedAt());
    statement->executeUpdate();
    const auto entryId = lastInsertId(connection);
    return getEntry(connection, entryId, entry.getOwnerUserName());
}

std::vector<MerchantReviewEntryModel> MerchantReviewDao::listEntries(
    const std::string& ownerUserName,
    const std::string& viewerUserName,
    unsigned int limit) const
{
    auto pooled = DatabaseConnectionPool::instance().acquire();
    sql::Connection* connection = pooled.operator->();
    std::unique_ptr<sql::PreparedStatement> statement(
        connection->prepareStatement(
            "SELECT entryId FROM merchantReviewEntry "
            "WHERE ownerUserName = ? AND status = 0 "
            "ORDER BY likeCount DESC, dislikeCount ASC, commentCount DESC, "
            "entryId DESC LIMIT ?"));
    statement->setString(1, ownerUserName);
    statement->setUInt(2, limit);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    std::vector<std::uint64_t> ids;
    while (result->next()) ids.push_back(result->getUInt64("entryId"));

    std::vector<MerchantReviewEntryModel> entries;
    entries.reserve(ids.size());
    for (const auto entryId : ids)
    {
        entries.push_back(getEntry(connection, entryId, viewerUserName));
    }
    return entries;
}

MerchantReviewEntryModel MerchantReviewDao::setReaction(
    std::uint64_t entryId,
    const std::string& userName,
    std::uint8_t reactionType,
    std::uint64_t now) const
{
    auto pooled = DatabaseConnectionPool::instance().acquire();
    sql::Connection* connection = pooled.operator->();
    connection->setAutoCommit(false);
    try
    {
        lockEntry(connection, entryId);
        std::unique_ptr<sql::PreparedStatement> findStatement(
            connection->prepareStatement(
                "SELECT reactionId, reactionType, createdAt FROM "
                "merchantReviewReaction WHERE entryId = ? AND userName = ? "
                "LIMIT 1 FOR UPDATE"));
        findStatement->setUInt64(1, entryId);
        findStatement->setString(2, userName);
        std::unique_ptr<sql::ResultSet> found(findStatement->executeQuery());

        MerchantReviewReactionModel reaction;
        reaction.setEntryId(entryId);
        reaction.setUserName(userName);
        reaction.setReactionType(reactionType);
        reaction.setUpdatedAt(now);
        if (found->next())
        {
            reaction.setReactionId(found->getUInt64("reactionId"));
            reaction.setCreatedAt(found->getUInt64("createdAt"));
        }
        else
        {
            reaction.setCreatedAt(now);
        }

        if (reactionType == 0)
        {
            std::unique_ptr<sql::PreparedStatement> removeStatement(
                connection->prepareStatement(
                    "DELETE FROM merchantReviewReaction "
                    "WHERE entryId = ? AND userName = ?"));
            removeStatement->setUInt64(1, entryId);
            removeStatement->setString(2, userName);
            removeStatement->executeUpdate();
        }
        else
        {
            std::unique_ptr<sql::PreparedStatement> saveStatement(
                connection->prepareStatement(
                    "INSERT INTO merchantReviewReaction "
                    "(entryId, userName, reactionType, createdAt, updatedAt) "
                    "VALUES (?, ?, ?, ?, ?) ON DUPLICATE KEY UPDATE "
                    "reactionType = VALUES(reactionType), "
                    "updatedAt = VALUES(updatedAt)"));
            saveStatement->setUInt64(1, reaction.getEntryId());
            saveStatement->setString(2, reaction.getUserName());
            saveStatement->setUInt(3, reaction.getReactionType());
            saveStatement->setUInt64(4, reaction.getCreatedAt());
            saveStatement->setUInt64(5, reaction.getUpdatedAt());
            saveStatement->executeUpdate();
        }

        std::unique_ptr<sql::PreparedStatement> countStatement(
            connection->prepareStatement(
                "UPDATE merchantReviewEntry SET "
                "likeCount = (SELECT COUNT(*) FROM merchantReviewReaction "
                "WHERE entryId = ? AND reactionType = 1), "
                "dislikeCount = (SELECT COUNT(*) FROM merchantReviewReaction "
                "WHERE entryId = ? AND reactionType = 2), updatedAt = ? "
                "WHERE entryId = ?"));
        countStatement->setUInt64(1, entryId);
        countStatement->setUInt64(2, entryId);
        countStatement->setUInt64(3, now);
        countStatement->setUInt64(4, entryId);
        countStatement->executeUpdate();

        connection->commit();
        connection->setAutoCommit(true);
        return getEntry(connection, entryId, userName);
    }
    catch (...)
    {
        rollbackQuietly(connection);
        throw;
    }
}

MerchantReviewEntryModel MerchantReviewDao::addComment(
    std::uint64_t entryId,
    const std::string& userName,
    const std::string& content,
    std::uint64_t now) const
{
    auto pooled = DatabaseConnectionPool::instance().acquire();
    sql::Connection* connection = pooled.operator->();
    connection->setAutoCommit(false);
    try
    {
        lockEntry(connection, entryId);
        MerchantReviewCommentModel comment;
        comment.setEntryId(entryId);
        comment.setUserName(userName);
        comment.setContent(content);
        comment.setCreatedAt(now);
        comment.setUpdatedAt(now);
        std::unique_ptr<sql::PreparedStatement> insertStatement(
            connection->prepareStatement(
                "INSERT INTO merchantReviewComment "
                "(entryId, userName, content, status, createdAt, updatedAt) "
                "VALUES (?, ?, ?, 0, ?, ?)"));
        insertStatement->setUInt64(1, comment.getEntryId());
        insertStatement->setString(2, comment.getUserName());
        insertStatement->setString(3, comment.getContent());
        insertStatement->setUInt64(4, comment.getCreatedAt());
        insertStatement->setUInt64(5, comment.getUpdatedAt());
        insertStatement->executeUpdate();

        std::unique_ptr<sql::PreparedStatement> countStatement(
            connection->prepareStatement(
                "UPDATE merchantReviewEntry SET "
                "commentCount = (SELECT COUNT(*) FROM merchantReviewComment "
                "WHERE entryId = ? AND status = 0), updatedAt = ? "
                "WHERE entryId = ?"));
        countStatement->setUInt64(1, entryId);
        countStatement->setUInt64(2, now);
        countStatement->setUInt64(3, entryId);
        countStatement->executeUpdate();

        connection->commit();
        connection->setAutoCommit(true);
        return getEntry(connection, entryId, userName);
    }
    catch (...)
    {
        rollbackQuietly(connection);
        throw;
    }
}

MerchantReviewEntryModel MerchantReviewDao::getEntry(
    sql::Connection* connection,
    std::uint64_t entryId,
    const std::string& viewerUserName) const
{
    std::unique_ptr<sql::PreparedStatement> statement(
        connection->prepareStatement(
            "SELECT e.*, COALESCE((SELECT r.reactionType FROM "
            "merchantReviewReaction r WHERE r.entryId = e.entryId "
            "AND r.userName = ? LIMIT 1), 0) AS viewerReaction "
            "FROM merchantReviewEntry e WHERE e.entryId = ? AND e.status = 0 "
            "LIMIT 1"));
    statement->setString(1, viewerUserName);
    statement->setUInt64(2, entryId);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    if (!result->next()) throw std::runtime_error("Merchant review not found");
    auto entry = readEntry(*result);
    entry.setComments(getComments(connection, entryId));
    return entry;
}

std::vector<MerchantReviewCommentModel> MerchantReviewDao::getComments(
    sql::Connection* connection,
    std::uint64_t entryId) const
{
    std::unique_ptr<sql::PreparedStatement> statement(
        connection->prepareStatement(
            "SELECT c.commentId, c.entryId, c.userName, c.content, c.status, "
            "c.createdAt, c.updatedAt, c.deletedAt, "
            "COALESCE(NULLIF(u.nickName, ''), c.userName) AS displayName "
            "FROM merchantReviewComment c LEFT JOIN userinfo u "
            "ON u.userName = c.userName WHERE c.entryId = ? AND c.status = 0 "
            "ORDER BY c.createdAt ASC"));
    statement->setUInt64(1, entryId);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    std::vector<MerchantReviewCommentModel> comments;
    while (result->next())
    {
        MerchantReviewCommentModel comment;
        comment.setCommentId(result->getUInt64("commentId"));
        comment.setEntryId(result->getUInt64("entryId"));
        comment.setUserName(result->getString("userName").asStdString());
        comment.setContent(result->getString("content").asStdString());
        comment.setStatus(static_cast<std::uint8_t>(result->getUInt("status")));
        comment.setCreatedAt(result->getUInt64("createdAt"));
        comment.setUpdatedAt(result->getUInt64("updatedAt"));
        if (!result->isNull("deletedAt"))
            comment.setDeletedAt(result->getUInt64("deletedAt"));
        comment.setDisplayName(result->getString("displayName").asStdString());
        comments.push_back(std::move(comment));
    }
    return comments;
}

void MerchantReviewDao::lockEntry(sql::Connection* connection,
                                  std::uint64_t entryId) const
{
    std::unique_ptr<sql::PreparedStatement> statement(
        connection->prepareStatement(
            "SELECT entryId FROM merchantReviewEntry "
            "WHERE entryId = ? AND status = 0 FOR UPDATE"));
    statement->setUInt64(1, entryId);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    if (!result->next()) throw std::runtime_error("Merchant review not found");
}
