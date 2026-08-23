#pragma once

#include <drogon/HttpController.h>

class GroupResourceController : public drogon::HttpController<GroupResourceController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(GroupResourceController::upload, "/api/group/resource/upload", drogon::Post);
    ADD_METHOD_TO(GroupResourceController::list, "/api/group/resource/list", drogon::Get);
    ADD_METHOD_TO(GroupResourceController::download, "/api/group/resource/download", drogon::Get);
    ADD_METHOD_TO(GroupResourceController::remove, "/api/group/resource/delete", drogon::Post);
    METHOD_LIST_END

    void upload(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void list(const drogon::HttpRequestPtr& req,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void download(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void remove(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};
