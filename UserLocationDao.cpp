#include "UserLocationDao.h"
#include <memory>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include "DatabaseConnectionPool.h"

bool UserLocationDao::upsert(const UserLocationModel& item) const {
    auto connection = DatabaseConnectionPool::instance().acquire();
    std::unique_ptr<sql::PreparedStatement> statement(connection->prepareStatement(
        "INSERT INTO userLocation (userName, latitude, longitude, accuracy, updatedAt) VALUES (?, ?, ?, ?, ?) "
        "ON DUPLICATE KEY UPDATE latitude=VALUES(latitude), longitude=VALUES(longitude), accuracy=VALUES(accuracy), updatedAt=VALUES(updatedAt)"));
    statement->setString(1, item.getUserName()); statement->setDouble(2, item.getLatitude());
    statement->setDouble(3, item.getLongitude()); statement->setDouble(4, item.getAccuracy());
    statement->setUInt64(5, item.getUpdatedAt()); return statement->executeUpdate() > 0;
}

std::optional<UserLocationModel> UserLocationDao::get(const std::string& userName) const {
    auto connection = DatabaseConnectionPool::instance().acquire();
    std::unique_ptr<sql::PreparedStatement> statement(connection->prepareStatement(
        "SELECT userName, latitude, longitude, accuracy, updatedAt FROM userLocation WHERE userName=? LIMIT 1"));
    statement->setString(1, userName); std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    if (!result->next()) return std::nullopt;
    UserLocationModel item; item.setUserName(result->getString("userName").asStdString());
    item.setLatitude(result->getDouble("latitude")); item.setLongitude(result->getDouble("longitude"));
    item.setAccuracy(result->getDouble("accuracy")); item.setUpdatedAt(result->getUInt64("updatedAt")); return item;
}

bool UserLocationDao::clear(const std::string& userName) const {
    auto connection = DatabaseConnectionPool::instance().acquire();
    std::unique_ptr<sql::PreparedStatement> statement(connection->prepareStatement("DELETE FROM userLocation WHERE userName=?"));
    statement->setString(1, userName); statement->executeUpdate(); return true;
}
