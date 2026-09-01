#pragma once

#include <string>
#include <vector>
#include <json/json.h>

class JPushService
{
public:
    static void pushToUsers(const std::vector<std::string>& userNames,
                            const std::string& title,
                            const std::string& body,
                            const Json::Value& extras = Json::Value(),
                            bool allowSound = true,
                            bool allowVibration = true);
};
