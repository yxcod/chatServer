#include "UserSpaceService.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "SpaceGuestbookMessageModel.h"
#include "UserSpaceDao.h"
#include "UserBlockDao.h"

namespace
{
Json::Value response(int code, const std::string& message)
{
    Json::Value value(Json::objectValue);
    value["code"] = code;
    value["message"] = message;
    return value;
}

Json::Value successWithData(Json::Value data)
{
    Json::Value value = response(100, "success");
    value["data"] = std::move(data);
    return value;
}

std::string trim(const std::string& value)
{
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return {};
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

std::size_t utf8CharacterCount(const std::string& value)
{
    return static_cast<std::size_t>(std::count_if(
        value.begin(), value.end(), [](unsigned char byte) {
            return (byte & 0xC0U) != 0x80U;
        }));
}

std::uint64_t currentTimeMillis()
{
    using namespace std::chrono;
    return static_cast<std::uint64_t>(duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count());
}

std::uint64_t readUInt64(const Json::Value& value)
{
    if (value.isUInt64()) return value.asUInt64();
    if (value.isInt64() && value.asInt64() > 0)
        return static_cast<std::uint64_t>(value.asInt64());
    if (value.isString() && !value.asString().empty())
        return std::stoull(value.asString());
    return 0;
}

Json::Value messageToJson(const SpaceGuestbookMessageModel& message)
{
    Json::Value value(Json::objectValue);
    value["messageId"] = Json::UInt64(message.getMessageId());
    value["ownerUserName"] = message.getOwnerUserName();
    value["authorUserName"] = message.getAuthorUserName();
    value["authorNickName"] = message.getAuthorNickName();
    value["authorAvatar"] = message.getAuthorAvatar();
    value["content"] = message.getContent();
    value["createdAt"] = Json::UInt64(message.getCreatedAt());
    return value;
}
}

Json::Value UserSpaceService::detail(const std::string& userName,
                                     const Json::Value& request) const
{
    const auto owner = trim(request.get("targetUserName", userName).asString());
    if (owner.empty() || owner.size() > 50)
        return response(101, "Invalid target user");
    const auto requestedLimit = request.get("messageLimit", 30).asUInt();
    const auto limit = std::max(1U, std::min(requestedLimit, 100U));
    try
    {
        if (owner != userName &&
            UserBlockDao().isBlockedEitherDirection(userName, owner))
            return response(403, "Access denied by blacklist");
        const auto space = UserSpaceDao().getSpace(owner);
        const auto messages = UserSpaceDao().listMessages(owner, limit);
        Json::Value messageItems(Json::arrayValue);
        for (const auto& message : messages)
            messageItems.append(messageToJson(message));
        Json::Value data(Json::objectValue);
        data["ownerUserName"] = owner;
        data["isOwner"] = owner == userName;
        data["coverImageUrl"] = space.getCoverImageUrl();
        data["messages"] = std::move(messageItems);
        return successWithData(std::move(data));
    }
    catch (const std::exception& error)
    {
        std::cerr << "Load user space failed: " << error.what() << '\n';
        return response(102, "Failed to load user space");
    }
}

Json::Value UserSpaceService::updateCover(const std::string& userName,
                                          const Json::Value& request) const
{
    const auto coverUrl = trim(request.get("coverImageUrl", "").asString());
    if (coverUrl.size() > 2048)
        return response(101, "Cover image URL is too long");
    try
    {
        const auto space = UserSpaceDao().updateCover(
            userName, coverUrl, currentTimeMillis());
        Json::Value data(Json::objectValue);
        data["ownerUserName"] = space.getOwnerUserName();
        data["coverImageUrl"] = space.getCoverImageUrl();
        return successWithData(std::move(data));
    }
    catch (const std::exception& error)
    {
        std::cerr << "Update space cover failed: " << error.what() << '\n';
        return response(102, "Failed to update space cover");
    }
}

Json::Value UserSpaceService::addMessage(const std::string& userName,
                                         const Json::Value& request) const
{
    const auto owner = trim(request.get("targetUserName", "").asString());
    const auto content = trim(request.get("content", "").asString());
    if (owner.empty() || owner.size() > 50 || content.empty() ||
        utf8CharacterCount(content) > 200)
        return response(101, "Invalid space message");
    try
    {
        if (owner != userName &&
            UserBlockDao().isBlockedEitherDirection(userName, owner))
            return response(403, "Access denied by blacklist");
        return successWithData(messageToJson(UserSpaceDao().addMessage(
            owner, userName, content, currentTimeMillis())));
    }
    catch (const std::exception& error)
    {
        std::cerr << "Add space message failed: " << error.what() << '\n';
        return response(102, "Failed to add space message");
    }
}

Json::Value UserSpaceService::deleteMessage(const std::string& userName,
                                            const Json::Value& request) const
{
    try
    {
        const auto messageId = readUInt64(request["messageId"]);
        if (messageId == 0) return response(101, "Invalid message id");
        if (!UserSpaceDao().deleteMessage(
                messageId, userName, currentTimeMillis()))
            return response(103, "Message not found or permission denied");
        return successWithData(Json::Value(Json::objectValue));
    }
    catch (const std::exception& error)
    {
        std::cerr << "Delete space message failed: " << error.what() << '\n';
        return response(102, "Failed to delete space message");
    }
}
