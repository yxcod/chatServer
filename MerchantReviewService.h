#pragma once

#include <string>

#include <json/json.h>

class MerchantReviewService
{
public:
    Json::Value addEntry(const std::string& userName,
                         const Json::Value& request) const;
    Json::Value listEntries(const std::string& userName,
                            const Json::Value& request) const;
    Json::Value setReaction(const std::string& userName,
                            const Json::Value& request) const;
    Json::Value addComment(const std::string& userName,
                           const Json::Value& request) const;
    Json::Value removeEntry(const std::string& userName,
                            const Json::Value& request) const;
};
