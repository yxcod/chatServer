#include "UserLocationController.h"
#include <cmath>
#include <json/json.h>
#include "FriendRelationDao.h"
#include "Logger.h"
#include "UserLocationDao.h"

namespace {
drogon::HttpResponsePtr response(int code, const std::string& message) {
    Json::Value value; value["code"] = code; value["message"] = message;
    return drogon::HttpResponse::newHttpJsonResponse(value);
}
bool acceptedFriends(const std::string& left, const std::string& right) {
    const auto relation = FriendRelationDao().getFriendRelation(left, right);
    return relation.getId() > 0 && relation.getStatus() == FriendRelation::RelationStatus::ACCEPTED;
}
double radians(double degrees) { return degrees * 3.14159265358979323846 / 180.0; }
double distanceMeters(const UserLocationModel& a, const UserLocationModel& b) {
    constexpr double radius = 6371000.0;
    const auto lat = radians(b.getLatitude() - a.getLatitude());
    const auto lon = radians(b.getLongitude() - a.getLongitude());
    const auto value = std::sin(lat/2)*std::sin(lat/2) + std::cos(radians(a.getLatitude())) *
        std::cos(radians(b.getLatitude())) * std::sin(lon/2)*std::sin(lon/2);
    return radius * 2 * std::atan2(std::sqrt(value), std::sqrt(1-value));
}
}

void UserLocationController::update(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    const auto json = req->getJsonObject(); if (!json) { callback(response(400, "Invalid JSON")); return; }
    const auto userName = (*json)["userName"].asString(); const auto latitude = (*json)["latitude"].asDouble();
    const auto longitude = (*json)["longitude"].asDouble(); const auto accuracy = (*json)["accuracy"].asDouble();
    if (userName.empty() || latitude < -90 || latitude > 90 || longitude < -180 || longitude > 180 || accuracy < 0) { callback(response(400, "Invalid location")); return; }
    try { UserLocationModel item; item.setUserName(userName); item.setLatitude(latitude); item.setLongitude(longitude);
        item.setAccuracy(accuracy); item.setUpdatedAt(Logger::GetInstance().getcurrentTime());
        if (!UserLocationDao().upsert(item)) { callback(response(500, "Update failed")); return; }
        Json::Value value; value["code"] = 100; value["updatedAt"] = Json::UInt64(item.getUpdatedAt()); callback(drogon::HttpResponse::newHttpJsonResponse(value));
    } catch (...) { callback(response(500, "Update failed")); }
}

void UserLocationController::distance(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    const auto json = req->getJsonObject(); if (!json) { callback(response(400, "Invalid JSON")); return; }
    const auto userName = (*json)["userName"].asString(); const auto peer = (*json)["peerUserName"].asString();
    if (userName.empty() || peer.empty() || !acceptedFriends(userName, peer)) { callback(response(403, "Friend relationship required")); return; }
    try { const auto mine = UserLocationDao().get(userName); const auto theirs = UserLocationDao().get(peer);
        Json::Value value; value["code"] = 100; value["available"] = mine.has_value() && theirs.has_value();
        if (mine && theirs) { value["distanceMeters"] = static_cast<Json::UInt64>(std::llround(distanceMeters(*mine, *theirs)));
            value["peerUpdatedAt"] = Json::UInt64(theirs->getUpdatedAt()); }
        callback(drogon::HttpResponse::newHttpJsonResponse(value));
    } catch (...) { callback(response(500, "Distance failed")); }
}

void UserLocationController::clear(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    const auto json = req->getJsonObject(); if (!json || (*json)["userName"].asString().empty()) { callback(response(400, "Invalid user")); return; }
    try { UserLocationDao().clear((*json)["userName"].asString()); callback(response(100, "success")); }
    catch (...) { callback(response(500, "Clear failed")); }
}
