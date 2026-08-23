#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "GroupResourceModel.h"

class GroupResourceDao
{
public:
    std::uint64_t insert(GroupResourceModel& resource) const;
    std::vector<GroupResourceModel> list(std::uint64_t groupId,
                                         std::uint8_t resourceType) const;
    std::optional<GroupResourceModel> get(std::uint64_t resourceId) const;
    bool markDeleted(std::uint64_t resourceId,
                     const std::string& deletedBy,
                     std::uint64_t deletedAt) const;
};
