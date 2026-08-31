#pragma once

#include <string>
#include <vector>

#include "UserBlockModel.h"

class UserBlockDao
{
public:
    bool add(const std::string& blockerUserName,
             const std::string& blockedUserName,
             std::uint64_t createdAt) const;
    bool remove(const std::string& blockerUserName,
                const std::string& blockedUserName) const;
    bool isBlockedBy(const std::string& blockerUserName,
                     const std::string& blockedUserName) const;
    bool isBlockedEitherDirection(const std::string& leftUserName,
                                  const std::string& rightUserName) const;
    std::vector<UserBlockModel> list(const std::string& blockerUserName) const;
};
