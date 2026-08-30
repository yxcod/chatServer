#include "AgoraRtcTokenController.h"

#include <optional>

#include "AgoraRtcTokenService.h"
#include "FriendRelationDao.h"
#include "GroupMemberDao.h"
#include "JwtTokenUtil.h"
#include "Logger.h"

namespace {
std::optional<std::string> authenticatedUser(const drogon::HttpRequestPtr& request) {
    static JwtTokenUtil tokenUtil(
        "c9bb708f526d420ea88d83cd316d662921646869efaf425eb150ab99d20f48bc");
    const auto token = tokenUtil.extractBearerToken(request);
    if (!token || !tokenUtil.verifyToken(*token)) return std::nullopt;
    const auto payload = tokenUtil.parsePayload(*token);
    const auto user = payload.find("userId");
    return user == payload.end() || user->second.empty()
        ? std::nullopt : std::optional<std::string>(user->second);
}
drogon::HttpResponsePtr failure(int code, const std::string& message,
                                 drogon::HttpStatusCode status) {
    Json::Value body(Json::objectValue);
    body["code"] = code;
    body["message"] = message;
    auto result = drogon::HttpResponse::newHttpJsonResponse(body);
    result->setStatusCode(status);
    return result;
}
}

void AgoraRtcTokenController::issue(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    const auto user = authenticatedUser(request);
    const auto json = request->getJsonObject();
    if (!user) {
        callback(failure(401, "Unauthorized", drogon::k401Unauthorized));
        return;
    }
    if (!json) {
        callback(failure(400, "Invalid JSON", drogon::k400BadRequest));
        return;
    }
    try {
        const auto kind = (*json)["callKind"].asString();
        bool allowed = false;
        if (kind == "private") {
            const auto peer = (*json)["peerId"].asString();
            allowed = !peer.empty() && peer != *user &&
                FriendRelationDao().hasAcceptedRelation(*user, peer);
        } else if (kind == "group") {
            const auto groupId = (*json)["groupId"].asUInt64();
            allowed = groupId > 0 && GroupMemberDao().isUserInGroup(groupId, *user);
        }
        if (!allowed) {
            callback(failure(403, "Call membership validation failed",
                             drogon::k403Forbidden));
            return;
        }
        const auto credential = AgoraRtcTokenService().issue(
            *user, (*json)["channelName"].asString());
        Json::Value body(Json::objectValue);
        body["code"] = 100;
        body["appId"] = credential.appId;
        body["token"] = credential.token;
        body["uid"] = credential.uid;
        body["expiresAt"] = Json::UInt64(credential.expiresAt);
        callback(drogon::HttpResponse::newHttpJsonResponse(body));
    } catch (const std::exception& error) {
        Logger::GetInstance().error(
            std::string("Agora RTC token issue failed: ") + error.what());
        callback(failure(503, "Video calling service is not configured",
                         drogon::k503ServiceUnavailable));
    }
}
