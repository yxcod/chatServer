#pragma once

#include <functional>
#include <optional>
#include <string>

#include <drogon/HttpController.h>

#include "JwtTokenUtil.h"
#include "MerchantReviewService.h"

class MerchantReviewController
    : public drogon::HttpController<MerchantReviewController>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(MerchantReviewController::addEntry,
                      "/api/merchantReview/add", drogon::Post);
        ADD_METHOD_TO(MerchantReviewController::listEntries,
                      "/api/merchantReview/list", drogon::Post);
        ADD_METHOD_TO(MerchantReviewController::setReaction,
                      "/api/merchantReview/reaction", drogon::Post);
        ADD_METHOD_TO(MerchantReviewController::addComment,
                      "/api/merchantReview/comment", drogon::Post);
        ADD_METHOD_TO(MerchantReviewController::removeComment,
                      "/api/merchantReview/comment/remove", drogon::Post);
        ADD_METHOD_TO(MerchantReviewController::removeEntry,
                      "/api/merchantReview/remove", drogon::Post);
    METHOD_LIST_END

    void addEntry(const drogon::HttpRequestPtr& request,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        handle(request, std::move(callback), &MerchantReviewService::addEntry);
    }

    void listEntries(const drogon::HttpRequestPtr& request,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        handle(request, std::move(callback), &MerchantReviewService::listEntries);
    }

    void setReaction(const drogon::HttpRequestPtr& request,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        handle(request, std::move(callback), &MerchantReviewService::setReaction);
    }

    void addComment(const drogon::HttpRequestPtr& request,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        handle(request, std::move(callback), &MerchantReviewService::addComment);
    }

    void removeEntry(const drogon::HttpRequestPtr& request,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        handle(request, std::move(callback), &MerchantReviewService::removeEntry);
    }

    void removeComment(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        handle(request, std::move(callback),
               &MerchantReviewService::removeComment);
    }

private:
    using ServiceMethod = Json::Value (MerchantReviewService::*)(
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
        if (!token || !tokenUtil().verifyToken(*token)) return std::nullopt;
        const auto payload = tokenUtil().parsePayload(*token);
        const auto user = payload.find("userId");
        if (user == payload.end() || user->second.empty()) return std::nullopt;
        return user->second;
    }

    static void handle(
        const drogon::HttpRequestPtr& request,
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
        MerchantReviewService service;
        callback(drogon::HttpResponse::newHttpJsonResponse(
            (service.*method)(*userName, *json)));
    }
};
