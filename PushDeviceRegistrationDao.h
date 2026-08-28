#pragma once

#include <string>
#include <vector>
#include "PushDeviceRegistrationModel.h"

class PushDeviceRegistrationDao
{
public:
    bool upsert(const PushDeviceRegistrationModel& registration) const;
    bool updateAppForeground(const std::string& userName,
                             const std::string& registrationId,
                             bool foreground,
                             std::uint64_t updatedAt) const;
    bool disable(const std::string& userName,
                 const std::string& registrationId,
                 std::uint64_t updatedAt) const;
    std::vector<PushDeviceRegistrationModel> getBackgroundDevices(
        const std::vector<std::string>& userNames) const;
};
