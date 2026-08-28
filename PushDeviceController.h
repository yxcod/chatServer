#pragma once

#include <drogon/HttpController.h>

class PushDeviceController : public drogon::HttpController<PushDeviceController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(PushDeviceController::registerDevice, "/api/push/register", drogon::Post);
    ADD_METHOD_TO(PushDeviceController::updateState, "/api/push/appState", drogon::Post);
    ADD_METHOD_TO(PushDeviceController::unregisterDevice, "/api/push/unregister", drogon::Post);
    METHOD_LIST_END

    void registerDevice(const drogon::HttpRequestPtr&,
                        std::function<void(const drogon::HttpResponsePtr&)>&&);
    void updateState(const drogon::HttpRequestPtr&,
                     std::function<void(const drogon::HttpResponsePtr&)>&&);
    void unregisterDevice(const drogon::HttpRequestPtr&,
                          std::function<void(const drogon::HttpResponsePtr&)>&&);
};
