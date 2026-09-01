#include "GroupResourceController.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <filesystem>
#include <sstream>

#include <drogon/MultiPart.h>
#include <json/json.h>

#include "GroupMemberDao.h"
#include "GroupResourceDao.h"
#include "Logger.h"

namespace
{
constexpr std::size_t kMaxPhotoBytes = 5ULL * 1024 * 1024;
constexpr std::size_t kMaxAlbumVideoBytes = 300ULL * 1024 * 1024;
constexpr std::size_t kMaxFileBytes = 300ULL * 1024 * 1024;
const std::filesystem::path kResourceRoot = "./groupResourceData";
std::atomic<std::uint64_t> sequence{0};

drogon::HttpResponsePtr reply(int code, const std::string& message,
                              drogon::HttpStatusCode status = drogon::k200OK)
{
    Json::Value value;
    value["code"] = code;
    value["message"] = message;
    auto response = drogon::HttpResponse::newHttpJsonResponse(value);
    response->setStatusCode(status);
    return response;
}

std::uint64_t uintValue(const std::string& value)
{
    try { return value.empty() ? 0 : std::stoull(value); }
    catch (...) { return 0; }
}

bool safeOriginalName(const std::string& value)
{
    if (value.empty() || value.size() > 255) return false;
    return std::none_of(value.begin(), value.end(), [](unsigned char ch) {
        return ch < 0x20 || ch == 0x7f || ch == '/' || ch == '\\';
    });
}

std::string lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string photoMime(std::string_view data)
{
    if (data.size() >= 3 && static_cast<unsigned char>(data[0]) == 0xff &&
        static_cast<unsigned char>(data[1]) == 0xd8 &&
        static_cast<unsigned char>(data[2]) == 0xff) return "image/jpeg";
    static constexpr unsigned char png[] = {0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a};
    if (data.size() >= sizeof(png) && std::equal(std::begin(png), std::end(png),
        reinterpret_cast<const unsigned char*>(data.data()))) return "image/png";
    if (data.size() >= 12 && data.substr(0,4) == "RIFF" && data.substr(8,4) == "WEBP") return "image/webp";
    return {};
}

std::string mimeFor(const std::string& name, std::string_view data)
{
    const auto image = photoMime(data);
    if (!image.empty()) return image;
    const auto ext = lower(std::filesystem::path(name).extension().string());
    if ((ext == ".mp4" || ext == ".m4v") && data.size() >= 12 && data.substr(4,4) == "ftyp") return "video/mp4";
    if (ext == ".mov" && data.size() >= 12 && data.substr(4,4) == "ftyp") return "video/quicktime";
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".txt") return "text/plain";
    if (ext == ".zip") return "application/zip";
    return "application/octet-stream";
}

bool isAlbumMime(const std::string& mime)
{
    return mime.rfind("image/", 0) == 0 || mime == "video/mp4" ||
        mime == "video/quicktime";
}

Json::Value toJson(const GroupResourceModel& item, const std::string& viewer,
                   bool manager)
{
    Json::Value value;
    value["resourceId"] = Json::UInt64(item.getResourceId());
    value["groupId"] = Json::UInt64(item.getGroupId());
    value["resourceType"] = item.getResourceType();
    value["originalName"] = item.getOriginalName();
    value["mimeType"] = item.getMimeType();
    value["fileSize"] = Json::UInt64(item.getFileSize());
    value["uploaderId"] = item.getUploaderId();
    value["createdAt"] = Json::UInt64(item.getCreatedAt());
    value["canDelete"] = manager || viewer == item.getUploaderId();
    return value;
}
}

void GroupResourceController::upload(const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    const auto groupId = uintValue(req->getParameter("groupId"));
    const auto userName = req->getParameter("userName");
    const auto type = static_cast<std::uint8_t>(uintValue(req->getParameter("resourceType")));
    if (!groupId || userName.empty() || (type != 1 && type != 2) ||
        !GroupMemberDao().isUserInGroup(groupId, userName))
    {
        callback(reply(403, "Not an active group member", drogon::k403Forbidden)); return;
    }
    drogon::MultiPartParser parser;
    if (parser.parse(req) != 0 || parser.getFiles().size() != 1)
    {
        callback(reply(400, "Exactly one file is required", drogon::k400BadRequest)); return;
    }
    const auto& file = parser.getFiles().front();
    const auto originalName = file.getFileName();
    const auto size = file.fileLength();
    if (!safeOriginalName(originalName) || size == 0)
    {
        callback(reply(400, "Invalid file", drogon::k400BadRequest)); return;
    }
    const auto mime = mimeFor(originalName, file.fileContent());
    if (type == 2 && !isAlbumMime(mime))
    {
        callback(reply(415, "Album only accepts JPEG, PNG, WebP, MP4, M4V or MOV", drogon::k415UnsupportedMediaType)); return;
    }
    const auto limit = type == 1 ? kMaxFileBytes :
        (mime.rfind("video/", 0) == 0 ? kMaxAlbumVideoBytes : kMaxPhotoBytes);
    if (size > limit)
    {
        callback(reply(413, mime.rfind("video/", 0) == 0 ?
            "Video exceeds 300MB" : "Photo exceeds 5MB",
            drogon::k413RequestEntityTooLarge)); return;
    }
    auto extension = lower(std::filesystem::path(originalName).extension().string());
    if (extension.size() > 12 || !std::all_of(extension.begin() + (extension.empty() ? 0 : 1), extension.end(), [](unsigned char ch) { return std::isalnum(ch) != 0; })) extension.clear();
    const auto now = Logger::GetInstance().getcurrentTime();
    const auto storedName = std::to_string(now) + "_" + std::to_string(sequence.fetch_add(1)) + extension;
    const auto directory = kResourceRoot / std::to_string(groupId);
    const auto target = directory / storedName;
    const auto temporary = target.string() + ".uploading";
    try
    {
        std::filesystem::create_directories(directory);
        if (file.saveAs(temporary) != 0) throw std::runtime_error("save failed");
        std::filesystem::rename(temporary, target);
        GroupResourceModel resource;
        resource.setGroupId(groupId); resource.setResourceType(type);
        resource.setOriginalName(originalName); resource.setStoredName(storedName);
        resource.setMimeType(mime); resource.setFileSize(size);
        resource.setUploaderId(userName); resource.setCreatedAt(now);
        if (!GroupResourceDao().insert(resource)) throw std::runtime_error("database insert failed");
        Json::Value value; value["code"] = 100; value["data"] = toJson(resource, userName, false);
        callback(drogon::HttpResponse::newHttpJsonResponse(value));
    }
    catch (const std::exception& error)
    {
        std::error_code ignored; std::filesystem::remove(temporary, ignored); std::filesystem::remove(target, ignored);
        Logger::GetInstance().error(std::string("group resource upload failed: ") + error.what());
        callback(reply(500, "Upload failed", drogon::k500InternalServerError));
    }
}

void GroupResourceController::list(const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    const auto groupId = uintValue(req->getParameter("groupId"));
    const auto userName = req->getParameter("userName");
    const auto type = static_cast<std::uint8_t>(uintValue(req->getParameter("resourceType")));
    const auto role = GroupMemberDao().getActiveMemberRole(groupId, userName);
    if (!role || (type != 1 && type != 2)) { callback(reply(403, "Not an active group member", drogon::k403Forbidden)); return; }
    try
    {
        Json::Value items(Json::arrayValue);
        for (const auto& item : GroupResourceDao().list(groupId, type)) items.append(toJson(item, userName, *role >= 1));
        Json::Value value; value["code"] = 100; value["items"] = std::move(items);
        callback(drogon::HttpResponse::newHttpJsonResponse(value));
    }
    catch (...) { callback(reply(500, "List failed", drogon::k500InternalServerError)); }
}

void GroupResourceController::download(const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    const auto resourceId = uintValue(req->getParameter("resourceId"));
    const auto userName = req->getParameter("userName");
    try
    {
        const auto resource = GroupResourceDao().get(resourceId);
        if (!resource || !GroupMemberDao().isUserInGroup(resource->getGroupId(), userName)) { callback(reply(404, "Resource not found", drogon::k404NotFound)); return; }
        const auto path = kResourceRoot / std::to_string(resource->getGroupId()) / resource->getStoredName();
        if (!std::filesystem::is_regular_file(path)) { callback(reply(404, "Resource file not found", drogon::k404NotFound)); return; }
        auto response = drogon::HttpResponse::newFileResponse(path.string(), "", drogon::CT_CUSTOM, resource->getMimeType(), req);
        response->addHeader("Accept-Ranges", "bytes");
        response->addHeader("X-Content-Type-Options", "nosniff");
        if (resource->getResourceType() == 1 && resource->getMimeType().rfind("video/", 0) != 0) response->addHeader("Content-Disposition", "attachment; filename=resource");
        callback(response);
    }
    catch (...) { callback(reply(500, "Download failed", drogon::k500InternalServerError)); }
}

void GroupResourceController::remove(const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    const auto json = req->getJsonObject();
    if (!json) { callback(reply(400, "Invalid JSON", drogon::k400BadRequest)); return; }
    const auto resourceId = (*json)["resourceId"].asUInt64();
    const auto operatorId = (*json)["userName"].asString();
    try
    {
        const auto resource = GroupResourceDao().get(resourceId);
        if (!resource) { callback(reply(404, "Resource not found", drogon::k404NotFound)); return; }
        const auto role = GroupMemberDao().getActiveMemberRole(resource->getGroupId(), operatorId);
        if (!role || (operatorId != resource->getUploaderId() && *role < 1)) { callback(reply(403, "No permission to delete", drogon::k403Forbidden)); return; }
        if (!GroupResourceDao().markDeleted(resourceId, operatorId, Logger::GetInstance().getcurrentTime())) { callback(reply(409, "Already deleted", drogon::k409Conflict)); return; }
        std::error_code ignored;
        std::filesystem::remove(kResourceRoot / std::to_string(resource->getGroupId()) / resource->getStoredName(), ignored);
        callback(reply(100, "success"));
    }
    catch (...) { callback(reply(500, "Delete failed", drogon::k500InternalServerError)); }
}
