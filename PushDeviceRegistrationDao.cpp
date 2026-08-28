#include "PushDeviceRegistrationDao.h"

#include <memory>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include "DatabaseConnectionPool.h"

namespace
{
PushDeviceRegistrationModel mapRegistration(sql::ResultSet& result)
{
    PushDeviceRegistrationModel item;
    item.setId(result.getUInt64("id"));
    item.setUserName(result.getString("userName").asStdString());
    item.setRegistrationId(result.getString("registrationId").asStdString());
    item.setPlatform(result.getString("platform").asStdString());
    item.setDeviceId(result.getString("deviceId").asStdString());
    item.setEnabled(result.getBoolean("enabled"));
    item.setAppForeground(result.getBoolean("appForeground"));
    item.setBannerEnabled(result.getBoolean("bannerEnabled"));
    item.setSoundEnabled(result.getBoolean("soundEnabled"));
    item.setVibrationEnabled(result.getBoolean("vibrationEnabled"));
    item.setCreatedAt(result.getUInt64("createdAt"));
    item.setUpdatedAt(result.getUInt64("updatedAt"));
    return item;
}
}

bool PushDeviceRegistrationDao::upsert(
    const PushDeviceRegistrationModel& item) const
{
    auto connection = DatabaseConnectionPool::instance().acquire();
    std::unique_ptr<sql::PreparedStatement> statement(connection->prepareStatement(
        "INSERT INTO pushDeviceRegistration "
        "(userName, registrationId, platform, deviceId, enabled, appForeground, "
        "bannerEnabled, soundEnabled, vibrationEnabled, createdAt, updatedAt) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
        "ON DUPLICATE KEY UPDATE userName=VALUES(userName), platform=VALUES(platform), "
        "deviceId=VALUES(deviceId), enabled=VALUES(enabled), "
        "appForeground=VALUES(appForeground), bannerEnabled=VALUES(bannerEnabled), "
        "soundEnabled=VALUES(soundEnabled), vibrationEnabled=VALUES(vibrationEnabled), "
        "updatedAt=VALUES(updatedAt)"));
    statement->setString(1, item.getUserName());
    statement->setString(2, item.getRegistrationId());
    statement->setString(3, item.getPlatform());
    statement->setString(4, item.getDeviceId());
    statement->setBoolean(5, item.isEnabled());
    statement->setBoolean(6, item.isAppForeground());
    statement->setBoolean(7, item.isBannerEnabled());
    statement->setBoolean(8, item.isSoundEnabled());
    statement->setBoolean(9, item.isVibrationEnabled());
    statement->setUInt64(10, item.getCreatedAt());
    statement->setUInt64(11, item.getUpdatedAt());
    return statement->executeUpdate() > 0;
}

bool PushDeviceRegistrationDao::updateAppForeground(
    const std::string& userName,
    const std::string& registrationId,
    bool foreground,
    std::uint64_t updatedAt) const
{
    auto connection = DatabaseConnectionPool::instance().acquire();
    std::unique_ptr<sql::PreparedStatement> statement(connection->prepareStatement(
        "UPDATE pushDeviceRegistration SET appForeground=?, updatedAt=? "
        "WHERE userName=? AND registrationId=? AND enabled=1"));
    statement->setBoolean(1, foreground);
    statement->setUInt64(2, updatedAt);
    statement->setString(3, userName);
    statement->setString(4, registrationId);
    return statement->executeUpdate() >= 0;
}

bool PushDeviceRegistrationDao::disable(
    const std::string& userName,
    const std::string& registrationId,
    std::uint64_t updatedAt) const
{
    auto connection = DatabaseConnectionPool::instance().acquire();
    std::unique_ptr<sql::PreparedStatement> statement(connection->prepareStatement(
        "UPDATE pushDeviceRegistration SET enabled=0, appForeground=0, updatedAt=? "
        "WHERE userName=? AND registrationId=?"));
    statement->setUInt64(1, updatedAt);
    statement->setString(2, userName);
    statement->setString(3, registrationId);
    return statement->executeUpdate() >= 0;
}

std::vector<PushDeviceRegistrationModel>
PushDeviceRegistrationDao::getBackgroundDevices(
    const std::vector<std::string>& userNames) const
{
    std::vector<PushDeviceRegistrationModel> registrations;
    if (userNames.empty()) return registrations;
    std::string placeholders;
    for (std::size_t index = 0; index < userNames.size(); ++index)
    {
        if (index > 0) placeholders += ',';
        placeholders += '?';
    }
    auto connection = DatabaseConnectionPool::instance().acquire();
    std::unique_ptr<sql::PreparedStatement> statement(connection->prepareStatement(
        "SELECT id, userName, registrationId, platform, deviceId, enabled, "
        "appForeground, bannerEnabled, soundEnabled, vibrationEnabled, "
        "createdAt, updatedAt FROM pushDeviceRegistration "
        "WHERE enabled=1 AND appForeground=0 AND userName IN (" +
        placeholders + ")"));
    for (std::size_t index = 0; index < userNames.size(); ++index)
        statement->setString(static_cast<unsigned int>(index + 1), userNames[index]);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    while (result->next()) registrations.push_back(mapRegistration(*result));
    return registrations;
}
