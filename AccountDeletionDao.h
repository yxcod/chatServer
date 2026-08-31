#pragma once

#include <cstdint>
#include <string>

enum class AccountDeletionResult
{
    Success,
    OwnsActiveGroup,
    AccountUnavailable,
    Failed,
};

class AccountDeletionDao
{
public:
    AccountDeletionResult deleteAccount(
        const std::string& userName,
        std::uint64_t deletedAt) const;
};
