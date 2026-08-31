#include "UserBlockDao.h"

#include <memory>
#include <stdexcept>

#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>

#include "Logger.h"

bool UserBlockDao::add(const std::string& blockerUserName,
                       const std::string& blockedUserName,
                       std::uint64_t createdAt) const
{
    auto connection = Logger::GetInstance().createConnection();
    if (!connection) throw std::runtime_error("Database connection is null");
    std::unique_ptr<sql::PreparedStatement> statement(connection->prepareStatement(
        "INSERT INTO userBlock (blockerUserName, blockedUserName, createdAt) "
        "VALUES (?, ?, ?) ON DUPLICATE KEY UPDATE createdAt = VALUES(createdAt)"));
    statement->setString(1, blockerUserName);
    statement->setString(2, blockedUserName);
    statement->setUInt64(3, createdAt);
    return statement->executeUpdate() > 0;
}

bool UserBlockDao::remove(const std::string& blockerUserName,
                          const std::string& blockedUserName) const
{
    auto connection = Logger::GetInstance().createConnection();
    if (!connection) throw std::runtime_error("Database connection is null");
    std::unique_ptr<sql::PreparedStatement> statement(connection->prepareStatement(
        "DELETE FROM userBlock WHERE blockerUserName = ? AND blockedUserName = ?"));
    statement->setString(1, blockerUserName);
    statement->setString(2, blockedUserName);
    return statement->executeUpdate() > 0;
}

bool UserBlockDao::isBlockedBy(const std::string& blockerUserName,
                               const std::string& blockedUserName) const
{
    if (blockerUserName.empty() || blockedUserName.empty()) return false;
    auto connection = Logger::GetInstance().createConnection();
    if (!connection) throw std::runtime_error("Database connection is null");
    std::unique_ptr<sql::PreparedStatement> statement(connection->prepareStatement(
        "SELECT 1 FROM userBlock WHERE blockerUserName = ? AND blockedUserName = ? LIMIT 1"));
    statement->setString(1, blockerUserName);
    statement->setString(2, blockedUserName);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    return result->next();
}

bool UserBlockDao::isBlockedEitherDirection(const std::string& leftUserName,
                                            const std::string& rightUserName) const
{
    if (leftUserName.empty() || rightUserName.empty()) return false;
    auto connection = Logger::GetInstance().createConnection();
    if (!connection) throw std::runtime_error("Database connection is null");
    std::unique_ptr<sql::PreparedStatement> statement(connection->prepareStatement(
        "SELECT 1 FROM userBlock WHERE (blockerUserName = ? AND blockedUserName = ?) "
        "OR (blockerUserName = ? AND blockedUserName = ?) LIMIT 1"));
    statement->setString(1, leftUserName);
    statement->setString(2, rightUserName);
    statement->setString(3, rightUserName);
    statement->setString(4, leftUserName);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    return result->next();
}

std::vector<UserBlockModel> UserBlockDao::list(
    const std::string& blockerUserName) const
{
    auto connection = Logger::GetInstance().createConnection();
    if (!connection) throw std::runtime_error("Database connection is null");
    std::unique_ptr<sql::PreparedStatement> statement(connection->prepareStatement(
        "SELECT id, blockerUserName, blockedUserName, createdAt FROM userBlock "
        "WHERE blockerUserName = ? ORDER BY createdAt DESC, id DESC"));
    statement->setString(1, blockerUserName);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    std::vector<UserBlockModel> items;
    while (result->next())
    {
        UserBlockModel item;
        item.setId(result->getUInt64("id"));
        item.setBlockerUserName(result->getString("blockerUserName"));
        item.setBlockedUserName(result->getString("blockedUserName"));
        item.setCreatedAt(result->getUInt64("createdAt"));
        items.push_back(std::move(item));
    }
    return items;
}
