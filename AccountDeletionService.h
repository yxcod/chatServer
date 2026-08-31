#pragma once

#include <json/json.h>
#include <string>

class AccountDeletionService
{
public:
    Json::Value deleteAccount(
        const std::string& userName,
        const std::string& password) const;
};
