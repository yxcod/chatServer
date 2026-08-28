#include "PushDeviceController.h"

#include <json/json.h>
#include "Logger.h"
#include "PushDeviceRegistrationDao.h"

namespace
{
drogon::HttpResponsePtr result(int code, const std::string& message)
{
    Json::Value value;
    value["code"] = code;
    value["message"] = message;
    return drogon::HttpResponse::newHttpJsonResponse(value);
}
}

void PushDeviceController::registerDevice(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    const auto json = request->getJsonObject();
    if (!json)
    {
        callback(result(400, "Invalid JSON"));
        return;
    }
    PushDeviceRegistrationModel item;
    item.setUserName((*json)["userName"].asString());
    item.setRegistrationId((*json)["registrationId"].asString());
    item.setPlatform((*json).get("platform", "android").asString());
    item.setDeviceId((*json).get("deviceId", "").asString());
    item.setEnabled(true);
    item.setAppForeground((*json).get("appForeground", true).asBool());
    item.setBannerEnabled((*json).get("bannerEnabled", true).asBool());
    item.setSoundEnabled((*json).get("soundEnabled", true).asBool());
    item.setVibrationEnabled((*json).get("vibrationEnabled", true).asBool());
    const auto now = Logger::GetInstance().getcurrentTime();
    item.setCreatedAt(now);
    item.setUpdatedAt(now);
    if (item.getUserName().empty() || item.getRegistrationId().empty() ||
        item.getPlatform() != "android")
    {
        callback(result(400, "Invalid push registration"));
        return;
    }
    try
    {
        callback(PushDeviceRegistrationDao().upsert(item)
            ? result(100, "success") : result(500, "Registration failed"));
    }
    catch (const std::exception& error)
    {
        Logger::GetInstance().error(
            std::string("Push registration failed: ") + error.what());
        callback(result(500, "Registration failed"));
    }
}

void PushDeviceController::updateState(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    const auto json = request->getJsonObject();
    if (!json || (*json)["userName"].asString().empty() ||
        (*json)["registrationId"].asString().empty())
    {
        callback(result(400, "Invalid state update"));
        return;
    }
    try
    {
        PushDeviceRegistrationDao().updateAppForeground(
            (*json)["userName"].asString(),
            (*json)["registrationId"].asString(),
            (*json).get("appForeground", false).asBool(),
            Logger::GetInstance().getcurrentTime());
        callback(result(100, "success"));
    }
    catch (const std::exception& error)
    {
        Logger::GetInstance().error(
            std::string("Push state update failed: ") + error.what());
        callback(result(500, "State update failed"));
    }
}

void PushDeviceController::unregisterDevice(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    const auto json = request->getJsonObject();
    if (!json || (*json)["userName"].asString().empty() ||
        (*json)["registrationId"].asString().empty())
    {
        callback(result(400, "Invalid unregister request"));
        return;
    }
    try
    {
        PushDeviceRegistrationDao().disable(
            (*json)["userName"].asString(),
            (*json)["registrationId"].asString(),
            Logger::GetInstance().getcurrentTime());
        callback(result(100, "success"));
    }
    catch (const std::exception& error)
    {
        Logger::GetInstance().error(
            std::string("Push unregister failed: ") + error.what());
        callback(result(500, "Unregister failed"));
    }
}
