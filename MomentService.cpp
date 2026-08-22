#include "MomentService.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "MomentDao.h"

namespace
{
Json::Value response(int code, const std::string& message)
{
    Json::Value value(Json::objectValue);
    value["code"] = code;
    value["message"] = message;
    return value;
}

std::string trim(const std::string& value)
{
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) {
        return std::isspace(c) != 0;
    }).base();
    return begin >= end ? std::string() : std::string(begin, end);
}

std::uint64_t readUInt64(const Json::Value& value)
{
    if (value.isUInt64()) return value.asUInt64();
    if (value.isInt64() && value.asInt64() > 0)
    {
        return static_cast<std::uint64_t>(value.asInt64());
    }
    if (value.isString() && !value.asString().empty())
    {
        return std::stoull(value.asString());
    }
    return 0;
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

Json::Value successWithData(Json::Value data)
{
    Json::Value value = response(100, "success");
    value["data"] = std::move(data);
    return value;
}
}

Json::Value MomentService::publish(const std::string& userName,
                                   const Json::Value& request) const
{
    const std::string content = trim(request.get("content", "").asString());
    const Json::Value mediaUrls = request["mediaUrls"];
    if (content.empty() && (!mediaUrls.isArray() || mediaUrls.empty()))
    {
        return response(101, "Moment content or media is required");
    }
    if (utf8CharacterCount(content) > 1000)
    {
        return response(101, "Moment content is too long");
    }

    MomentCreateData data;
    data.authorUserName = userName;
    data.content = content;
    data.visibility = request.get("visibility", 0).asUInt();
    if (data.visibility > 2)
    {
        return response(101, "Invalid visibility");
    }
    data.locationName = trim(request.get("location", "").asString());
    if (utf8CharacterCount(data.locationName) > 100)
    {
        return response(101, "Location is too long");
    }
    data.clientRequestId = request.get("clientRequestId", "").asString();
    if (data.clientRequestId.size() > 64)
    {
        return response(101, "Invalid client request id");
    }
    if (mediaUrls.isArray())
    {
        if (mediaUrls.size() > 9)
        {
            return response(101, "At most 9 media files are allowed");
        }
        for (const auto& mediaUrl : mediaUrls)
        {
            const std::string url = mediaUrl.asString();
            if (url.empty() || url.size() > 1024)
            {
                return response(101, "Invalid media URL");
            }
            data.mediaUrls.push_back(url);
        }
    }
    data.createdAt = currentTimeMillis();

    try
    {
        return successWithData(MomentDao().createMoment(data));
    }
    catch (const std::exception& error)
    {
        std::cerr << "Publish moment failed: " << error.what() << '\n';
        return response(102, "Failed to publish moment");
    }
}

Json::Value MomentService::ownList(const std::string& userName,
                                   const Json::Value& request) const
{
    try
    {
        const auto beforeMomentId = readUInt64(request["beforeMomentId"]);
        const auto requestedLimit = request.get("limit", 30).asUInt();
        const auto limit = std::max(1U, std::min(requestedLimit, 50U));
        Json::Value result(Json::objectValue);
        result["items"] = MomentDao().getOwnMoments(userName, beforeMomentId, limit);
        result["hasMore"] = result["items"].size() == limit;
        return successWithData(std::move(result));
    }
    catch (const std::exception& error)
    {
        std::cerr << "Load moments failed: " << error.what() << '\n';
        return response(102, "Failed to load moments");
    }
}

Json::Value MomentService::toggleLike(const std::string& userName,
                                      const Json::Value& request) const
{
    try
    {
        const auto momentId = readUInt64(request["momentId"]);
        if (momentId == 0) return response(101, "Invalid moment id");
        return successWithData(MomentDao().toggleLike(
            momentId, userName, currentTimeMillis()));
    }
    catch (const std::exception& error)
    {
        std::cerr << "Toggle moment like failed: " << error.what() << '\n';
        return response(102, "Failed to update like");
    }
}

Json::Value MomentService::addComment(const std::string& userName,
                                      const Json::Value& request) const
{
    try
    {
        const auto momentId = readUInt64(request["momentId"]);
        const std::string content = trim(request.get("content", "").asString());
        if (momentId == 0 || content.empty() || utf8CharacterCount(content) > 1000)
        {
            return response(101, "Invalid comment");
        }
        return successWithData(MomentDao().addComment(
            momentId, userName, content, currentTimeMillis()));
    }
    catch (const std::exception& error)
    {
        std::cerr << "Add moment comment failed: " << error.what() << '\n';
        return response(102, "Failed to add comment");
    }
}
