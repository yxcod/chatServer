#pragma once

#include <functional>
#include <optional>
#include <string>

#include <drogon/HttpController.h>

#include "JwtTokenUtil.h"
#include "MomentService.h"

class MomentController : public drogon::HttpController<MomentController>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(MomentController::publish, "/api/moment/publish", drogon::Post);
        ADD_METHOD_TO(MomentController::ownList, "/api/moment/ownList", drogon::Post);
        ADD_METHOD_TO(MomentController::userList, "/api/moment/userList", drogon::Post);
        ADD_METHOD_TO(MomentController::toggleLike, "/api/moment/toggleLike", drogon::Post);
        ADD_METHOD_TO(MomentController::addComment, "/api/moment/comment", drogon::Post);
        ADD_METHOD_TO(MomentController::deleteMoment, "/api/moment/delete", drogon::Post);
        ADD_METHOD_TO(MomentController::notifications, "/api/moment/notifications", drogon::Post);
        ADD_METHOD_TO(MomentController::notificationUnreadCount, "/api/moment/notifications/unreadCount", drogon::Post);
        ADD_METHOD_TO(MomentController::markNotificationsRead, "/api/moment/notifications/readAll", drogon::Post);
    METHOD_LIST_END

    void publish(const drogon::HttpRequestPtr& request,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        handle(request, std::move(callback), &MomentService::publish);
    }

    void ownList(const drogon::HttpRequestPtr& request,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        handle(request, std::move(callback), &MomentService::ownList);
    }

    void toggleLike(const drogon::HttpRequestPtr& request,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        handle(request, std::move(callback), &MomentService::toggleLike);
    }

    void userList(const drogon::HttpRequestPtr& request,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        handle(request, std::move(callback), &MomentService::userList);
    }

    void addComment(const drogon::HttpRequestPtr& request,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        handle(request, std::move(callback), &MomentService::addComment);
    }

    void deleteMoment(const drogon::HttpRequestPtr& request,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        handle(request, std::move(callback), &MomentService::deleteMoment);
    }

    void notifications(const drogon::HttpRequestPtr& request,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        handle(request, std::move(callback), &MomentService::notifications);
    }

    void notificationUnreadCount(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        handle(request, std::move(callback),
               &MomentService::notificationUnreadCount);
    }

    void markNotificationsRead(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        handle(request, std::move(callback),
               &MomentService::markNotificationsRead);
    }

private:
    using ServiceMethod = Json::Value (MomentService::*)(
        const std::string&, const Json::Value&) const;

    static JwtTokenUtil& tokenUtil()
    {
        static JwtTokenUtil instance(
            "c9bb708f526d420ea88d83cd316d662921646869efaf425eb150ab99d20f48bc");
        return instance;
    }

    static std::optional<std::string> authenticatedUser(
        const drogon::HttpRequestPtr& request)
    {
        auto token = tokenUtil().extractBearerToken(request);
        if (!token || !tokenUtil().verifyToken(*token))
        {
            return std::nullopt;
        }
        const auto payload = tokenUtil().parsePayload(*token);
        const auto user = payload.find("userId");
        if (user == payload.end() || user->second.empty())
        {
            return std::nullopt;
        }
        return user->second;
    }

    static void handle(const drogon::HttpRequestPtr& request,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       ServiceMethod method)
    {
        const auto userName = authenticatedUser(request);
        if (!userName)
        {
            Json::Value body(Json::objectValue);
            body["code"] = 401;
            body["message"] = "Unauthorized";
            auto response = drogon::HttpResponse::newHttpJsonResponse(body);
            response->setStatusCode(drogon::k401Unauthorized);
            callback(response);
            return;
        }

        const auto json = request->getJsonObject();
        if (!json)
        {
            Json::Value body(Json::objectValue);
            body["code"] = 99;
            body["message"] = "Invalid JSON body";
            auto response = drogon::HttpResponse::newHttpJsonResponse(body);
            response->setStatusCode(drogon::k400BadRequest);
            callback(response);
            return;
        }

        MomentService service;
        callback(drogon::HttpResponse::newHttpJsonResponse(
            (service.*method)(*userName, *json)));
    }
};
