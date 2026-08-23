#pragma once
#include <drogon/HttpController.h>
class UserLocationController : public drogon::HttpController<UserLocationController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(UserLocationController::update, "/api/location/update", drogon::Post);
    ADD_METHOD_TO(UserLocationController::distance, "/api/location/distance", drogon::Post);
    ADD_METHOD_TO(UserLocationController::clear, "/api/location/clear", drogon::Post);
    METHOD_LIST_END
    void update(const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&);
    void distance(const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&);
    void clear(const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&);
};
