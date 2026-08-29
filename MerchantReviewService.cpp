#include "MerchantReviewService.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "MerchantReviewDao.h"
#include "MerchantReviewEntryModel.h"

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

bool safeImageName(const std::string& value)
{
    return value.empty() ||
           (value.size() <= 255 && value.find('/') == std::string::npos &&
            value.find('\\') == std::string::npos &&
            value.find("..") == std::string::npos);
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

std::string reactionName(std::uint8_t reaction)
{
    if (reaction == 1) return "like";
    if (reaction == 2) return "dislike";
    return "none";
}

std::uint8_t reactionValue(const std::string& reaction)
{
    if (reaction == "like") return 1;
    if (reaction == "dislike") return 2;
    if (reaction == "none") return 0;
    throw std::invalid_argument("Invalid reaction");
}

std::string jsonArrayToString(const Json::Value& value)
{
    if (!value.isArray() || value.empty()) return {};
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, value);
}

Json::Value parseImageUrls(const std::string& raw)
{
    Json::Value value(Json::arrayValue);
    if (raw.empty()) return value;
    Json::CharReaderBuilder builder;
    std::istringstream stream(raw);
    std::string errors;
    Json::Value parsed;
    if (Json::parseFromStream(builder, stream, &parsed, &errors) && parsed.isArray())
        return parsed;
    return value;
}

Json::Value commentToJson(const MerchantReviewCommentModel& comment)
{
    Json::Value value(Json::objectValue);
    value["id"] = Json::UInt64(comment.getCommentId());
    value["userId"] = comment.getUserName();
    value["displayName"] = comment.getDisplayName();
    value["avatarName"] = comment.getAvatarName();
    value["content"] = comment.getContent();
    value["imageName"] = comment.getImageName();
    value["createdAt"] = Json::UInt64(comment.getCreatedAt());
    return value;
}

Json::Value entryToJson(const MerchantReviewEntryModel& entry)
{
    Json::Value merchant(Json::objectValue);
    merchant["id"] = entry.getPoiId();
    merchant["name"] = entry.getMerchantName();
    merchant["address"] = entry.getAddress();
    merchant["category"] = entry.getCategory();
    merchant["distanceMeters"] = entry.hasDistanceMeters()
        ? Json::Value(entry.getDistanceMeters())
        : Json::Value();
    merchant["rating"] = entry.hasRating()
        ? Json::Value(entry.getRating())
        : Json::Value();
    merchant["imageUrl"] = entry.getImageUrl();
    merchant["imageUrls"] = parseImageUrls(entry.getImageUrlsJson());
    merchant["phone"] = entry.getPhone();
    merchant["openingHours"] = entry.getOpeningHours();
    merchant["price"] = entry.hasPrice()
        ? Json::Value(entry.getPrice())
        : Json::Value();
    merchant["detailUrl"] = entry.getDetailUrl();
    merchant["imageCount"] = entry.getImageCount();
    merchant["latitude"] = entry.hasLatitude()
        ? Json::Value(entry.getLatitude())
        : Json::Value();
    merchant["longitude"] = entry.hasLongitude()
        ? Json::Value(entry.getLongitude())
        : Json::Value();

    Json::Value comments(Json::arrayValue);
    for (const auto& comment : entry.getComments())
        comments.append(commentToJson(comment));

    Json::Value value(Json::objectValue);
    value["entryId"] = Json::UInt64(entry.getEntryId());
    value["ownerUserName"] = entry.getOwnerUserName();
    value["uploadedImages"] = parseImageUrls(entry.getUploadedImagesJson());
    value["merchant"] = std::move(merchant);
    value["addedAt"] = Json::UInt64(entry.getCreatedAt());
    value["likes"] = entry.getLikeCount();
    value["dislikes"] = entry.getDislikeCount();
    value["reaction"] = reactionName(entry.getViewerReaction());
    value["comments"] = std::move(comments);
    return value;
}

bool validOptionalString(const std::string& value, std::size_t maxBytes)
{
    return value.size() <= maxBytes;
}
}

Json::Value MerchantReviewService::addEntry(
    const std::string& userName,
    const Json::Value& request) const
{
    const Json::Value merchant = request["merchant"];
    if (!merchant.isObject()) return response(101, "Merchant is required");
    const std::string poiId = trim(merchant.get("id", "").asString());
    const std::string name = trim(merchant.get("name", "").asString());
    if (poiId.empty() || poiId.size() > 128 || name.empty() ||
        utf8CharacterCount(name) > 160)
    {
        return response(101, "Invalid merchant");
    }
    const std::string address = trim(merchant.get("address", "").asString());
    const std::string category = trim(merchant.get("category", "").asString());
    const std::string imageUrl = trim(merchant.get("imageUrl", "").asString());
    const std::string phone = trim(merchant.get("phone", "").asString());
    const std::string openingHours = trim(
        merchant.get("openingHours", "").asString());
    const std::string detailUrl = trim(merchant.get("detailUrl", "").asString());
    const Json::Value imageUrls = merchant["imageUrls"];
    if (!validOptionalString(address, 500) ||
        !validOptionalString(category, 200) ||
        !validOptionalString(imageUrl, 2048) ||
        !validOptionalString(phone, 100) ||
        !validOptionalString(openingHours, 500) ||
        !validOptionalString(detailUrl, 2048) ||
        (imageUrls.isArray() && imageUrls.size() > 20))
    {
        return response(101, "Merchant data is too long");
    }
    if (imageUrls.isArray())
    {
        for (const auto& item : imageUrls)
        {
            if (!item.isString() || item.asString().size() > 2048)
                return response(101, "Invalid merchant image URL");
        }
    }

    MerchantReviewEntryModel entry;
    entry.setOwnerUserName(userName);
    entry.setPoiId(poiId);
    entry.setMerchantName(name);
    entry.setAddress(address);
    entry.setCategory(category);
    entry.setImageUrl(imageUrl);
    entry.setImageUrlsJson(jsonArrayToString(imageUrls));
    entry.setPhone(phone);
    entry.setOpeningHours(openingHours);
    entry.setDetailUrl(detailUrl);
    entry.setImageCount(merchant.get("imageCount", 0).asUInt());
    if (merchant.isMember("distanceMeters") && merchant["distanceMeters"].isNumeric())
        entry.setDistanceMeters(merchant["distanceMeters"].asUInt());
    if (merchant.isMember("rating") && merchant["rating"].isNumeric())
        entry.setRating(merchant["rating"].asDouble());
    if (merchant.isMember("price") && merchant["price"].isNumeric())
        entry.setPrice(merchant["price"].asDouble());
    if (merchant.isMember("latitude") && merchant["latitude"].isNumeric())
        entry.setLatitude(merchant["latitude"].asDouble());
    if (merchant.isMember("longitude") && merchant["longitude"].isNumeric())
        entry.setLongitude(merchant["longitude"].asDouble());
    const auto now = currentTimeMillis();
    entry.setCreatedAt(now);
    entry.setUpdatedAt(now);

    try
    {
        return successWithData(entryToJson(
            MerchantReviewDao().addEntry(std::move(entry))));
    }
    catch (const std::exception& error)
    {
        std::cerr << "Add merchant review failed: " << error.what() << '\n';
        return response(102, "Failed to add merchant review");
    }
}

Json::Value MerchantReviewService::listEntries(
    const std::string& userName,
    const Json::Value& request) const
{
    const std::string targetUserName = trim(
        request.get("targetUserName", userName).asString());
    if (targetUserName.empty() || targetUserName.size() > 50)
        return response(101, "Invalid target user");
    const auto requestedLimit = request.get("limit", 100).asUInt();
    const auto limit = std::max(1U, std::min(requestedLimit, 200U));
    try
    {
        const auto entries = MerchantReviewDao().listEntries(
            targetUserName, userName, limit);
        Json::Value items(Json::arrayValue);
        for (const auto& entry : entries) items.append(entryToJson(entry));
        Json::Value data(Json::objectValue);
        data["items"] = std::move(items);
        return successWithData(std::move(data));
    }
    catch (const std::exception& error)
    {
        std::cerr << "List merchant reviews failed: " << error.what() << '\n';
        return response(102, "Failed to load merchant reviews");
    }
}

Json::Value MerchantReviewService::setReaction(
    const std::string& userName,
    const Json::Value& request) const
{
    try
    {
        const auto entryId = readUInt64(request["entryId"]);
        if (entryId == 0) return response(101, "Invalid entry id");
        const auto reaction = reactionValue(
            trim(request.get("reaction", "none").asString()));
        return successWithData(entryToJson(MerchantReviewDao().setReaction(
            entryId, userName, reaction, currentTimeMillis())));
    }
    catch (const std::invalid_argument&)
    {
        return response(101, "Invalid reaction");
    }
    catch (const std::exception& error)
    {
        std::cerr << "Set merchant reaction failed: " << error.what() << '\n';
        return response(102, "Failed to update merchant reaction");
    }
}

Json::Value MerchantReviewService::addComment(
    const std::string& userName,
    const Json::Value& request) const
{
    try
    {
        const auto entryId = readUInt64(request["entryId"]);
        const std::string content = trim(request.get("content", "").asString());
        const std::string imageName = trim(request.get("imageName", "").asString());
        if (entryId == 0 || (content.empty() && imageName.empty()) ||
            utf8CharacterCount(content) > 500 || !safeImageName(imageName))
            return response(101, "Invalid comment");
        return successWithData(entryToJson(MerchantReviewDao().addComment(
            entryId, userName, content, imageName, currentTimeMillis())));
    }
    catch (const std::exception& error)
    {
        std::cerr << "Add merchant review comment failed: " << error.what() << '\n';
        return response(102, "Failed to add merchant review comment");
    }
}

Json::Value MerchantReviewService::removeEntry(
    const std::string& userName,
    const Json::Value& request) const
{
    try
    {
        const auto entryId = readUInt64(request["entryId"]);
        if (entryId == 0) return response(101, "Invalid entry id");
        MerchantReviewDao().removeEntry(entryId, userName);
        Json::Value data(Json::objectValue);
        data["entryId"] = Json::UInt64(entryId);
        return successWithData(std::move(data));
    }
    catch (const std::exception& error)
    {
        std::cerr << "Remove merchant review failed: " << error.what() << '\n';
        return response(102, "Failed to remove merchant review");
    }
}

Json::Value MerchantReviewService::setUploadedImages(
    const std::string& userName,
    const Json::Value& request) const
{
    try
    {
        const auto entryId = readUInt64(request["entryId"]);
        const Json::Value imageNames = request["imageNames"];
        if (entryId == 0 || !imageNames.isArray() || imageNames.size() > 4)
            return response(101, "Invalid merchant images");
        Json::Value images(Json::arrayValue);
        for (const auto& item : imageNames)
        {
            if (!item.isString()) return response(101, "Invalid merchant image");
            const std::string imageName = trim(item.asString());
            if (imageName.empty() || !safeImageName(imageName))
                return response(101, "Invalid merchant image");
            Json::Value image(Json::objectValue);
            image["ownerId"] = userName;
            image["imageName"] = imageName;
            images.append(std::move(image));
        }
        return successWithData(entryToJson(MerchantReviewDao().setUploadedImages(
            entryId, userName, jsonArrayToString(images), currentTimeMillis())));
    }
    catch (const std::exception& error)
    {
        std::cerr << "Set merchant review images failed: " << error.what() << '\n';
        return response(102, "Failed to update merchant images");
    }
}

Json::Value MerchantReviewService::removeComment(
    const std::string& userName,
    const Json::Value& request) const
{
    try
    {
        const auto entryId = readUInt64(request["entryId"]);
        const auto commentId = readUInt64(request["commentId"]);
        if (entryId == 0 || commentId == 0)
            return response(101, "Invalid comment id");
        return successWithData(entryToJson(MerchantReviewDao().removeComment(
            entryId, commentId, userName, currentTimeMillis())));
    }
    catch (const std::exception& error)
    {
        std::cerr << "Remove merchant review comment failed: "
                  << error.what() << '\n';
        return response(102, "Failed to remove merchant review comment");
    }
}
