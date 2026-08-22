#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <json/json.h>
#include <jdbc/cppconn/connection.h>

struct MomentCreateData
{
    std::string authorUserName;
    std::string content;
    unsigned int visibility{0};
    std::string locationName;
    std::string clientRequestId;
    std::vector<std::string> mediaUrls;
    std::uint64_t createdAt{0};
};

class MomentDao
{
public:
    Json::Value createMoment(const MomentCreateData& data) const;
    Json::Value getOwnMoments(const std::string& userName,
                              std::uint64_t beforeMomentId,
                              unsigned int limit) const;
    Json::Value toggleLike(std::uint64_t momentId,
                           const std::string& userName,
                           std::uint64_t now) const;
    Json::Value addComment(std::uint64_t momentId,
                           const std::string& userName,
                           const std::string& content,
                           std::uint64_t now) const;

private:
    Json::Value getMoment(sql::Connection* connection,
                          std::uint64_t momentId,
                          const std::string& viewerUserName) const;
    void appendMedia(sql::Connection* connection,
                     std::uint64_t momentId,
                     Json::Value& moment) const;
    void appendComments(sql::Connection* connection,
                        std::uint64_t momentId,
                        Json::Value& moment) const;
};
