#include "AccountDeletionService.h"

#include "AccountDeletionDao.h"
#include "ChatManageController.h"
#include "EncryptUtil.h"
#include "HeartbeatManager.h"
#include "LoginDao.h"
#include "Logger.h"

namespace
{
Json::Value response(int code, const char* message)
{
    Json::Value value;
    value["code"] = code;
    value["message"] = message;
    return value;
}
}

Json::Value AccountDeletionService::deleteAccount(
    const std::string& userName,
    const std::string& password) const
{
    if (userName.empty() || password.empty())
        return response(99, u8"账号或密码不能为空");

    const LoginInfo login = LoginDao().loginAccount(userName);
    if (!login.found || login.isBan != 0)
        return response(102, u8"账号不存在或已注销");

    const OpenSslEncryptUtil encryptUtil;
    if (!encryptUtil.verifyPassword(password, login.password, login.salt))
        return response(101, u8"密码错误");

    const auto result = AccountDeletionDao().deleteAccount(
        userName, Logger::GetInstance().getcurrentTime());
    if (result == AccountDeletionResult::OwnsActiveGroup)
        return response(103, u8"请先解散或转让由你创建的群聊");
    if (result == AccountDeletionResult::AccountUnavailable)
        return response(102, u8"账号不存在或已注销");
    if (result != AccountDeletionResult::Success)
        return response(104, u8"账户注销失败，请稍后重试");

    HeartbeatManager::GetInstance().handleDisconnect(userName);
    if (auto* server = ChatWSServer::GetInstance(); server != nullptr)
        server->closeConnectionByUser(userName);
    return response(100, u8"账户已注销");
}
