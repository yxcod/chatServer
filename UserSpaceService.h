#pragma once

#include <string>

#include <json/json.h>

class UserSpaceService
{
public:
    Json::Value detail(const std::string& userName,
                       const Json::Value& request) const;
    Json::Value updateCover(const std::string& userName,
                            const Json::Value& request) const;
    Json::Value addMessage(const std::string& userName,
                           const Json::Value& request) const;
    Json::Value deleteMessage(const std::string& userName,
                              const Json::Value& request) const;
};
