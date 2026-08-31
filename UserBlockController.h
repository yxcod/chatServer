#pragma once

#include <functional>
#include <optional>
#include <string>
#include <utility>

#include <drogon/HttpController.h>

#include "JwtTokenUtil.h"
#include "UserBlockService.h"

class UserBlockController : public drogon::HttpController<UserBlockController>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(UserBlockController::add, "/api/blacklist/add", drogon::Post);
        ADD_METHOD_TO(UserBlockController::remove, "/api/blacklist/remove", drogon::Post);
        ADD_METHOD_TO(UserBlockController::list, "/api/blacklist/list", drogon::Post);
        ADD_METHOD_TO(UserBlockController::status, "/api/blacklist/status", drogon::Post);
    METHOD_LIST_END

    void add(const drogon::HttpRequestPtr& request,
             std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    { handle(request, std::move(callback), &UserBlockService::add); }
    void remove(const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    { handle(request, std::move(callback), &UserBlockService::remove); }
    void list(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    { handle(request, std::move(callback), &UserBlockService::list); }
    void status(const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    { handle(request, std::move(callback), &UserBlockService::status); }

private:
    using Method = Json::Value (UserBlockService::*)(
        const std::string&, const Json::Value&) const;

    static std::optional<std::string> authenticatedUser(
        const drogon::HttpRequestPtr& request)
    {
        static JwtTokenUtil tokenUtil(
            "c9bb708f526d420ea88d83cd316d662921646869efaf425eb150ab99d20f48bc");
        const auto token = tokenUtil.extractBearerToken(request);
        if (!token || !tokenUtil.verifyToken(*token)) return std::nullopt;
        const auto payload = tokenUtil.parsePayload(*token);
        const auto user = payload.find("userId");
        return user == payload.end() || user->second.empty()
            ? std::nullopt : std::optional<std::string>(user->second);
    }

    static void handle(const drogon::HttpRequestPtr& request,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       Method method)
    {
        const auto user = authenticatedUser(request);
        if (!user)
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
        const Json::Value empty(Json::objectValue);
        UserBlockService service;
        callback(drogon::HttpResponse::newHttpJsonResponse(
            (service.*method)(*user, json ? *json : empty)));
    }
};
