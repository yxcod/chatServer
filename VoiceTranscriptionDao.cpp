#include "VoiceTranscriptionDao.h"

#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <memory>

#include "DatabaseConnectionPool.h"

std::optional<VoiceTranscriptionModel> VoiceTranscriptionDao::find(
    const std::string& audioOwnerId,
    const std::string& audioName,
    const std::string& audioSha256,
    const std::string& engineType) const
{
    auto connection = DatabaseConnectionPool::instance().acquire();
    std::unique_ptr<sql::PreparedStatement> statement(connection->prepareStatement(
        "SELECT id, audioOwnerId, audioName, audioSha256, engineType, transcript, "
        "audioDurationMs, providerRequestId, createdAt, updatedAt "
        "FROM voiceTranscription WHERE audioOwnerId=? AND audioName=? "
        "AND audioSha256=? AND engineType=? LIMIT 1"));
    statement->setString(1, audioOwnerId);
    statement->setString(2, audioName);
    statement->setString(3, audioSha256);
    statement->setString(4, engineType);
    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
    if (!result->next()) return std::nullopt;

    VoiceTranscriptionModel item;
    item.setId(result->getUInt64("id"));
    item.setAudioOwnerId(result->getString("audioOwnerId").asStdString());
    item.setAudioName(result->getString("audioName").asStdString());
    item.setAudioSha256(result->getString("audioSha256").asStdString());
    item.setEngineType(result->getString("engineType").asStdString());
    item.setTranscript(result->getString("transcript").asStdString());
    item.setAudioDurationMs(result->getUInt("audioDurationMs"));
    item.setProviderRequestId(result->getString("providerRequestId").asStdString());
    item.setCreatedAt(result->getUInt64("createdAt"));
    item.setUpdatedAt(result->getUInt64("updatedAt"));
    return item;
}

bool VoiceTranscriptionDao::upsert(
    const VoiceTranscriptionModel& item) const
{
    auto connection = DatabaseConnectionPool::instance().acquire();
    std::unique_ptr<sql::PreparedStatement> statement(connection->prepareStatement(
        "INSERT INTO voiceTranscription (audioOwnerId, audioName, audioSha256, "
        "engineType, transcript, audioDurationMs, providerRequestId, createdAt, updatedAt) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?) ON DUPLICATE KEY UPDATE "
        "transcript=VALUES(transcript), audioDurationMs=VALUES(audioDurationMs), "
        "providerRequestId=VALUES(providerRequestId), updatedAt=VALUES(updatedAt)"));
    statement->setString(1, item.getAudioOwnerId());
    statement->setString(2, item.getAudioName());
    statement->setString(3, item.getAudioSha256());
    statement->setString(4, item.getEngineType());
    statement->setString(5, item.getTranscript());
    statement->setUInt(6, item.getAudioDurationMs());
    statement->setString(7, item.getProviderRequestId());
    statement->setUInt64(8, item.getCreatedAt());
    statement->setUInt64(9, item.getUpdatedAt());
    return statement->executeUpdate() > 0;
}
