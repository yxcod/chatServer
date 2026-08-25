#include "MomentService.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "MomentDao.h"
#include "MomentMediaModel.h"
#include "MomentModel.h"

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

std::string toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
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

Json::Value commentToJson(const MomentCommentModel& comment)
{
    Json::Value value(Json::objectValue);
    value["id"] = Json::UInt64(comment.getCommentId());
    value["userId"] = comment.getUserName();
    value["displayName"] = comment.getDisplayName();
    value["content"] = comment.getContent();
    value["createdAt"] = Json::UInt64(comment.getCreatedAt());
    return value;
}

Json::Value momentToJson(const MomentModel& moment)
{
    Json::Value value(Json::objectValue);
    value["id"] = Json::UInt64(moment.getMomentId());
    value["authorId"] = moment.getAuthorUserName();
    value["authorName"] = moment.getAuthorNickName();
    value["authorAvatarUrl"] = moment.getAuthorAvatar();
    value["content"] = moment.getContent();
    value["visibility"] = moment.getVisibility();
    value["location"] = moment.getLocationName().empty()
        ? Json::Value()
        : Json::Value(moment.getLocationName());
    value["likeCount"] = moment.getLikeCount();
    value["commentCount"] = moment.getCommentCount();
    value["isLiked"] = moment.isLikedByViewer();
    value["createdAt"] = Json::UInt64(moment.getCreatedAt());

    Json::Value media(Json::arrayValue);
    for (const auto& item : moment.getMedia())
    {
        media.append(item.getMediaUrl());
    }
    value["mediaPaths"] = std::move(media);

    Json::Value comments(Json::arrayValue);
    for (const auto& comment : moment.getComments())
    {
        comments.append(commentToJson(comment));
    }
    value["comments"] = std::move(comments);
    return value;
}

bool isSafeMediaName(const std::string& value)
{
    if (value.empty() || value == "." || value == ".." || value.size() > 180)
    {
        return false;
    }
    return std::none_of(value.begin(), value.end(), [](unsigned char ch) {
        return ch < 0x20 || ch == 0x7f || ch == '/' || ch == '\\' || ch == ':';
    });
}

std::string queryValue(const std::string& url, const std::string& key)
{
    const std::string marker = key + "=";
    const auto begin = url.find(marker);
    if (begin == std::string::npos) return {};
    const auto valueBegin = begin + marker.size();
    const auto end = url.find('&', valueBegin);
    return url.substr(valueBegin, end == std::string::npos
        ? std::string::npos
        : end - valueBegin);
}

void removeMomentMediaFiles(const std::string& userName,
                            const std::vector<std::string>& mediaUrls)
{
    namespace fs = std::filesystem;
    for (const auto& url : mediaUrls)
    {
        const bool image = url.find("/api/image/download") != std::string::npos;
        const bool video = url.find("/api/video/download") != std::string::npos;
        if (!image && !video) continue;
        const std::string fileName = queryValue(
            url, image ? "imageName" : "videoName");
        const std::string expectedPrefix = userName + "_moment_";
        if (!isSafeMediaName(fileName) || fileName.rfind(expectedPrefix, 0) != 0)
        {
            continue;
        }
        std::error_code error;
        fs::remove((image ? fs::path("./imageData") : fs::path("./videoData")) /
            userName / fileName, error);
        if (error)
        {
            std::cerr << "Remove moment media failed: " << error.message() << '\n';
        }
    }
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

    MomentModel moment;
    moment.setAuthorUserName(userName);
    moment.setContent(content);
    const auto visibility = request.get("visibility", 0).asUInt();
    if (visibility > 2)
    {
        return response(101, "Invalid visibility");
    }
    moment.setVisibility(static_cast<std::uint8_t>(visibility));
    moment.setLocationName(trim(request.get("location", "").asString()));
    if (utf8CharacterCount(moment.getLocationName()) > 100)
    {
        return response(101, "Location is too long");
    }
    moment.setClientRequestId(request.get("clientRequestId", "").asString());
    if (moment.getClientRequestId().size() > 64)
    {
        return response(101, "Invalid client request id");
    }
    std::vector<MomentMediaModel> media;
    if (mediaUrls.isArray())
    {
        if (mediaUrls.size() > 9)
        {
            return response(101, "At most 9 media files are allowed");
        }
        std::uint16_t sortOrder = 0;
        for (const auto& mediaUrl : mediaUrls)
        {
            const std::string url = mediaUrl.asString();
            if (url.empty() || url.size() > 1024)
            {
                return response(101, "Invalid media URL");
            }
            MomentMediaModel item;
            const std::string lowerUrl = toLower(url);
            const bool isVideo = lowerUrl.find("/api/video/download") != std::string::npos ||
                lowerUrl.find(".mp4") != std::string::npos ||
                lowerUrl.find(".mov") != std::string::npos ||
                lowerUrl.find(".m4v") != std::string::npos;
            item.setMediaType(isVideo ? 1 : 0);
            item.setMediaUrl(url);
            item.setSortOrder(sortOrder++);
            media.push_back(std::move(item));
        }
    }
    const auto now = currentTimeMillis();
    moment.setCreatedAt(now);
    moment.setUpdatedAt(now);
    for (auto& item : media) item.setCreatedAt(now);

    try
    {
        return successWithData(momentToJson(
            MomentDao().createMoment(std::move(moment), media)));
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
        const auto limit = requestedLimit < 1U
            ? 1U
            : (requestedLimit > 50U ? 50U : requestedLimit);
        const auto moments = MomentDao().getOwnMoments(
            userName, beforeMomentId, limit);
        Json::Value items(Json::arrayValue);
        for (const auto& moment : moments)
        {
            items.append(momentToJson(moment));
        }
        Json::Value result(Json::objectValue);
        result["items"] = std::move(items);
        result["hasMore"] = moments.size() == limit;
        return successWithData(std::move(result));
    }
    catch (const std::exception& error)
    {
        std::cerr << "Load moments failed: " << error.what() << '\n';
        return response(102, "Failed to load moments");
    }
}

Json::Value MomentService::userList(const std::string& userName,
                                    const Json::Value& request) const
{
    const std::string authorUserName = trim(
        request.get("targetUserName", "").asString());
    if (authorUserName.empty() || authorUserName.size() > 50)
    {
        return response(101, "Invalid target user");
    }

    try
    {
        const auto beforeMomentId = readUInt64(request["beforeMomentId"]);
        const auto requestedLimit = request.get("limit", 30).asUInt();
        const auto limit = requestedLimit < 1U
            ? 1U
            : (requestedLimit > 50U ? 50U : requestedLimit);
        const auto moments = MomentDao().getVisibleMoments(
            userName, authorUserName, beforeMomentId, limit);
        Json::Value items(Json::arrayValue);
        for (const auto& moment : moments)
        {
            items.append(momentToJson(moment));
        }
        Json::Value result(Json::objectValue);
        result["items"] = std::move(items);
        result["hasMore"] = moments.size() == limit;
        return successWithData(std::move(result));
    }
    catch (const std::exception& error)
    {
        std::cerr << "Load visible moments failed: " << error.what() << '\n';
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
        return successWithData(momentToJson(MomentDao().toggleLike(
            momentId, userName, currentTimeMillis())));
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
        return successWithData(momentToJson(MomentDao().addComment(
            momentId, userName, content, currentTimeMillis())));
    }
    catch (const std::exception& error)
    {
        std::cerr << "Add moment comment failed: " << error.what() << '\n';
        return response(102, "Failed to add comment");
    }
}

Json::Value MomentService::deleteMoment(const std::string& userName,
                                        const Json::Value& request) const
{
    try
    {
        const auto momentId = readUInt64(request["momentId"]);
        if (momentId == 0) return response(101, "Invalid moment id");
        const auto mediaUrls = MomentDao().deleteMoment(momentId, userName);
        removeMomentMediaFiles(userName, mediaUrls);
        Json::Value data(Json::objectValue);
        data["momentId"] = Json::UInt64(momentId);
        return successWithData(std::move(data));
    }
    catch (const std::exception& error)
    {
        std::cerr << "Delete moment failed: " << error.what() << '\n';
        return response(102, "Failed to delete moment");
    }
}
