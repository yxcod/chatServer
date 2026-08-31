#include "UserBlockService.h"

#include <iostream>

#include "FriendRelationDao.h"
#include "Logger.h"
#include "UserBlockDao.h"
#include "UserInfoDao.h"

namespace
{
Json::Value response(int code, const std::string& message)
{
    Json::Value value(Json::objectValue);
    value["code"] = code;
    value["message"] = message;
    return value;
}

std::string targetOf(const Json::Value& request)
{
    return request.get("targetUserName", "").asString();
}
}

Json::Value UserBlockService::add(const std::string& userName,
                                  const Json::Value& request) const
{
    const auto target = targetOf(request);
    if (target.empty() || target.size() > 50 || target == userName)
        return response(101, "Invalid target user");
    try
    {
        const auto user = UserInfoDao().getUserinfo(target);
        if (user.getUserAccount().empty()) return response(103, "User not found");
        UserBlockDao dao;
        dao.add(userName, target, Logger::GetInstance().getcurrentTime());
        // A block immediately invalidates accepted and pending relationships.
        FriendRelationDao().deleteFriendRelation(userName, target);
        Json::Value value = response(100, "success");
        value["data"]["targetUserName"] = target;
        value["data"]["blockedByMe"] = true;
        return value;
    }
    catch (const std::exception& error)
    {
        Logger::GetInstance().error(
            std::string("Add user block failed: ") + error.what());
        return response(102, "Failed to add user to blacklist");
    }
}

Json::Value UserBlockService::remove(const std::string& userName,
                                     const Json::Value& request) const
{
    const auto target = targetOf(request);
    if (target.empty() || target.size() > 50 || target == userName)
        return response(101, "Invalid target user");
    try
    {
        UserBlockDao().remove(userName, target);
        Json::Value value = response(100, "success");
        value["data"]["targetUserName"] = target;
        value["data"]["blockedByMe"] = false;
        return value;
    }
    catch (const std::exception& error)
    {
        Logger::GetInstance().error(
            std::string("Remove user block failed: ") + error.what());
        return response(102, "Failed to remove user from blacklist");
    }
}

Json::Value UserBlockService::list(const std::string& userName,
                                   const Json::Value&) const
{
    try
    {
        Json::Value items(Json::arrayValue);
        UserInfoDao userDao;
        for (const auto& block : UserBlockDao().list(userName))
        {
            const auto user = userDao.getUserinfo(block.getBlockedUserName());
            Json::Value item(Json::objectValue);
            item["userName"] = block.getBlockedUserName();
            item["nickName"] = user.getNickName();
            item["avatar"] = user.getAvatar();
            item["avatarVersion"] = Json::UInt64(user.getModifyTime());
            item["createdAt"] = Json::UInt64(block.getCreatedAt());
            items.append(std::move(item));
        }
        Json::Value value = response(100, "success");
        value["data"]["items"] = std::move(items);
        return value;
    }
    catch (const std::exception& error)
    {
        Logger::GetInstance().error(
            std::string("List user blocks failed: ") + error.what());
        return response(102, "Failed to load blacklist");
    }
}

Json::Value UserBlockService::status(const std::string& userName,
                                     const Json::Value& request) const
{
    const auto target = targetOf(request);
    if (target.empty() || target.size() > 50 || target == userName)
        return response(101, "Invalid target user");
    try
    {
        UserBlockDao dao;
        Json::Value value = response(100, "success");
        value["data"]["blockedByMe"] = dao.isBlockedBy(userName, target);
        value["data"]["blockedMe"] = dao.isBlockedBy(target, userName);
        return value;
    }
    catch (const std::exception& error)
    {
        Logger::GetInstance().error(
            std::string("Load user block status failed: ") + error.what());
        return response(102, "Failed to load blacklist status");
    }
}
