#include "AccountDeletionDao.h"

#include "DatabaseConnectionPool.h"
#include "Logger.h"

#include <memory>
#include <string>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>

namespace
{
void executeForUser(
    sql::Connection& connection,
    const std::string& statementText,
    const std::string& userName)
{
    std::unique_ptr<sql::PreparedStatement> statement(
        connection.prepareStatement(statementText));
    statement->setString(1, userName);
    statement->executeUpdate();
}

void executeForUserTwice(
    sql::Connection& connection,
    const std::string& statementText,
    const std::string& userName)
{
    std::unique_ptr<sql::PreparedStatement> statement(
        connection.prepareStatement(statementText));
    statement->setString(1, userName);
    statement->setString(2, userName);
    statement->executeUpdate();
}
}

AccountDeletionResult AccountDeletionDao::deleteAccount(
    const std::string& userName,
    std::uint64_t deletedAt) const
{
    if (userName.empty()) return AccountDeletionResult::AccountUnavailable;

    auto connection = DatabaseConnectionPool::instance().acquire();
    try
    {
        connection->setAutoCommit(false);

        std::unique_ptr<sql::PreparedStatement> ownerStatement(
            connection->prepareStatement(
                "SELECT 1 FROM groupChat "
                "WHERE creatorId = ? AND isActive = 1 LIMIT 1 FOR UPDATE"));
        ownerStatement->setString(1, userName);
        std::unique_ptr<sql::ResultSet> ownerResult(
            ownerStatement->executeQuery());
        if (ownerResult->next())
        {
            connection->rollback();
            connection->setAutoCommit(true);
            return AccountDeletionResult::OwnsActiveGroup;
        }

        std::unique_ptr<sql::PreparedStatement> accountStatement(
            connection->prepareStatement(
                "UPDATE login SET isBan = 2, password = '', salt = '' "
                "WHERE userAccount = ? AND isBan = 0"));
        accountStatement->setString(1, userName);
        if (accountStatement->executeUpdate() <= 0)
        {
            connection->rollback();
            connection->setAutoCommit(true);
            return AccountDeletionResult::AccountUnavailable;
        }

        // Keep historical chat rows for the other participant, but remove
        // identifying profile fields and all future social visibility.
        std::unique_ptr<sql::PreparedStatement> profileStatement(
            connection->prepareStatement(
                "UPDATE userinfo SET nickName = ?, avatar = '', gender = 0, "
                "region = '', signature = '', state = 0, modifyTime = ? "
                "WHERE userName = ?"));
        profileStatement->setString(1, u8"已注销用户");
        profileStatement->setUInt64(2, deletedAt);
        profileStatement->setString(3, userName);
        profileStatement->executeUpdate();

        std::unique_ptr<sql::PreparedStatement> friendStatement(
            connection->prepareStatement(
                "UPDATE friendrelation SET status = 5, updateTime = ? "
                "WHERE fromUserId = ? OR toUserId = ?"));
        friendStatement->setUInt64(1, deletedAt);
        friendStatement->setString(2, userName);
        friendStatement->setString(3, userName);
        friendStatement->executeUpdate();

        executeForUserTwice(*connection,
            "DELETE FROM userBlock "
            "WHERE blockerUserName = ? OR blockedUserName = ?",
            userName);
        executeForUser(*connection,
            "DELETE FROM userLocation WHERE userName = ?", userName);
        executeForUser(*connection,
            "DELETE FROM pushDeviceRegistration WHERE userName = ?", userName);

        executeForUserTwice(*connection,
            "DELETE FROM momentLike WHERE userName = ? OR momentId IN "
            "(SELECT momentId FROM moment WHERE authorUserName = ?)", userName);
        executeForUserTwice(*connection,
            "DELETE FROM momentComment WHERE userName = ? OR momentId IN "
            "(SELECT momentId FROM moment WHERE authorUserName = ?)", userName);
        executeForUser(*connection,
            "DELETE FROM momentMedia WHERE momentId IN "
            "(SELECT momentId FROM moment WHERE authorUserName = ?)", userName);
        executeForUser(*connection,
            "DELETE FROM moment WHERE authorUserName = ?", userName);

        executeForUser(*connection,
            "DELETE FROM merchantReviewReaction WHERE userName = ?", userName);
        executeForUser(*connection,
            "DELETE FROM merchantReviewComment WHERE userName = ?", userName);
        executeForUser(*connection,
            "DELETE FROM merchantReviewReaction WHERE entryId IN "
            "(SELECT entryId FROM merchantReviewEntry WHERE ownerUserName = ?)",
            userName);
        executeForUser(*connection,
            "DELETE FROM merchantReviewComment WHERE entryId IN "
            "(SELECT entryId FROM merchantReviewEntry WHERE ownerUserName = ?)",
            userName);
        executeForUser(*connection,
            "DELETE FROM merchantReviewEntry WHERE ownerUserName = ?", userName);
        std::unique_ptr<sql::Statement> reviewCountStatement(
            connection->createStatement());
        reviewCountStatement->executeUpdate(
            "UPDATE merchantReviewEntry e SET "
            "likeCount = (SELECT COUNT(*) FROM merchantReviewReaction r "
            "WHERE r.entryId = e.entryId AND r.reactionType = 1), "
            "dislikeCount = (SELECT COUNT(*) FROM merchantReviewReaction r "
            "WHERE r.entryId = e.entryId AND r.reactionType = 2), "
            "commentCount = (SELECT COUNT(*) FROM merchantReviewComment c "
            "WHERE c.entryId = e.entryId AND c.status = 0)");

        executeForUserTwice(*connection,
            "DELETE FROM spaceGuestbookMessage "
            "WHERE ownerUserName = ? OR authorUserName = ?", userName);
        executeForUser(*connection,
            "DELETE FROM userSpace WHERE ownerUserName = ?", userName);
        executeForUser(*connection,
            "DELETE FROM voiceTranscription WHERE audioOwnerId = ?", userName);
        executeForUserTwice(*connection,
            "DELETE FROM private_chat_history_visibility "
            "WHERE userId = ? OR peerUserId = ?", userName);
        executeForUser(*connection,
            "DELETE FROM groupMsgRead WHERE userId = ?", userName);

        std::unique_ptr<sql::PreparedStatement> quitGroupsStatement(
            connection->prepareStatement(
                "UPDATE groupMember SET isQuit = 1, quitTime = ?, isMuted = 0, "
                "mutedBy = '', mutedAt = 0 WHERE userId = ? AND isQuit = 0"));
        quitGroupsStatement->setUInt64(1, deletedAt);
        quitGroupsStatement->setString(2, userName);
        quitGroupsStatement->executeUpdate();

        // Hide conversations only for the deleted side. The peer retains their
        // own historical copy with an anonymous deleted-account profile.
        std::unique_ptr<sql::PreparedStatement> conversationStatement(
            connection->prepareStatement(
                "UPDATE conversations SET "
                "user1isVaild = CASE WHEN user1Id = ? THEN 0 ELSE user1isVaild END, "
                "user2isValid = CASE WHEN user2Id = ? THEN 0 ELSE user2isValid END, "
                "user1UnreadCount = CASE WHEN user1Id = ? THEN 0 ELSE user1UnreadCount END, "
                "user2UnreadCount = CASE WHEN user2Id = ? THEN 0 ELSE user2UnreadCount END "
                "WHERE user1Id = ? OR user2Id = ?"));
        for (unsigned int index = 1; index <= 6; ++index)
            conversationStatement->setString(index, userName);
        conversationStatement->executeUpdate();

        connection->commit();
        connection->setAutoCommit(true);
        return AccountDeletionResult::Success;
    }
    catch (const std::exception& error)
    {
        try
        {
            connection->rollback();
            connection->setAutoCommit(true);
        }
        catch (...) {}
        Logger::GetInstance().error(
            std::string("Account deletion failed for ") + userName +
            ": " + error.what());
    }
    catch (...)
    {
        try
        {
            connection->rollback();
            connection->setAutoCommit(true);
        }
        catch (...) {}
        Logger::GetInstance().error(
            std::string("Account deletion failed for ") + userName +
            ": unknown exception");
    }
    return AccountDeletionResult::Failed;
}
