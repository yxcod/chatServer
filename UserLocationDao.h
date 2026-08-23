#pragma once
#include <optional>
#include <string>
#include "UserLocationModel.h"
class UserLocationDao {
public:
    bool upsert(const UserLocationModel& location) const;
    std::optional<UserLocationModel> get(const std::string& userName) const;
    bool clear(const std::string& userName) const;
};
