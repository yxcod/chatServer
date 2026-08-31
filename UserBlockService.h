#pragma once

#include <string>
#include <json/json.h>

class UserBlockService
{
public:
    Json::Value add(const std::string& userName, const Json::Value& request) const;
    Json::Value remove(const std::string& userName, const Json::Value& request) const;
    Json::Value list(const std::string& userName, const Json::Value& request) const;
    Json::Value status(const std::string& userName, const Json::Value& request) const;
};
