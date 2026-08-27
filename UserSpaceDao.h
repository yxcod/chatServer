#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "SpaceGuestbookMessageModel.h"
#include "UserSpaceModel.h"

class UserSpaceDao
{
public:
    UserSpaceModel getSpace(const std::string& ownerUserName) const;
    UserSpaceModel updateCover(const std::string& ownerUserName,
                               const std::string& coverImageUrl,
                               std::uint64_t now) const;
    std::vector<SpaceGuestbookMessageModel> listMessages(
        const std::string& ownerUserName,
        unsigned int limit) const;
    SpaceGuestbookMessageModel addMessage(
        const std::string& ownerUserName,
        const std::string& authorUserName,
        const std::string& content,
        std::uint64_t now) const;
    bool deleteMessage(std::uint64_t messageId,
                       const std::string& operatorUserName,
                       std::uint64_t now) const;
};
