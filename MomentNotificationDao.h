#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "MomentNotificationModel.h"

class MomentNotificationDao
{
public:
    MomentNotificationModel create(
        const std::string& recipientUserName,
        const std::string& actorUserName,
        std::uint64_t momentId,
        std::uint8_t interactionType,
        const std::string& commentContent,
        std::uint64_t createdAt) const;
    std::vector<MomentNotificationModel> list(
        const std::string& recipientUserName,
        std::uint64_t beforeNotificationId,
        unsigned int limit) const;
    unsigned int unreadCount(const std::string& recipientUserName) const;
    unsigned int markAllRead(const std::string& recipientUserName,
                             std::uint64_t readAt) const;
};
