#pragma once
#include <string>
#include <ctime>  // 用于时间类型（数据库 DATETIME 对应 C++ tm 结构体）
#include <sstream>
#include <iomanip>
#include "Logger.h"
struct LoginInfo
{
    bool found{};
    std::string password;
    int isBan{};
    std::string salt;
};

class LoginDao
{
public:
    int registerAccount(const std::string& account,
        const std::string& password,
        const std::string& salt);

    LoginInfo loginAccount(const std::string& account);

    int changePassword(const std::string& account,
        const std::string& newPassword);
};

