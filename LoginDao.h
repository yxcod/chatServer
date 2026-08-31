#pragma once
#include <string>
#include <ctime>  // 用于时间类型（数据库 DATETIME 对应 C++ tm 结构体）
#include <sstream>
#include <iomanip>
#include <cstdint>
#include "Logger.h"
struct LoginInfo
{
    bool found{};
    std::string password;
    int isBan{};
    std::string salt;
    std::uint64_t sessionVersion{};
};

class LoginDao
{
public:
    int registerAccount(const std::string& account,
        const std::string& password,
        const std::string& salt);

    LoginInfo loginAccount(const std::string& account);

    bool isAccountActive(const std::string& account) const;

    std::uint64_t createSession(const std::string& account) const;

    bool isSessionActive(const std::string& account,
        std::uint64_t sessionVersion) const;

    int changePassword(const std::string& account,
        const std::string& newPassword);
};
