#pragma once

#include <string>

#include <json/json.h>

class MomentService
{
public:
    Json::Value publish(const std::string& userName, const Json::Value& request) const;
    Json::Value ownList(const std::string& userName, const Json::Value& request) const;
    Json::Value userList(const std::string& userName, const Json::Value& request) const;
    Json::Value toggleLike(const std::string& userName, const Json::Value& request) const;
    Json::Value addComment(const std::string& userName, const Json::Value& request) const;
    Json::Value deleteMoment(const std::string& userName, const Json::Value& request) const;
};
