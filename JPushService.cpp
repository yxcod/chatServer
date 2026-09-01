#include "JPushService.h"

#include <drogon/drogon.h>
#include <drogon/utils/Utilities.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <unordered_set>
#include "Logger.h"
#include "PushDeviceRegistrationDao.h"

namespace
{
struct JPushConfig
{
    std::string appKey;
    std::string masterSecret;
    bool valid() const { return !appKey.empty() && !masterSecret.empty(); }
};

JPushConfig loadConfig()
{
    static std::mutex mutex;
    static JPushConfig cached;
    static bool loaded = false;
    std::lock_guard<std::mutex> lock(mutex);
    if (loaded) return cached;
    loaded = true;
    const std::filesystem::path path =
        std::filesystem::current_path() / "config" / "jpush.local.json";
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        Logger::GetInstance().error(
            "JPush config missing: " + path.string());
        return cached;
    }
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errors;
    if (!Json::parseFromStream(builder, input, &root, &errors))
    {
        Logger::GetInstance().error("JPush config invalid JSON");
        return cached;
    }
    cached.appKey = root["appKey"].asString();
    cached.masterSecret = root["masterSecret"].asString();
    if (!cached.valid())
        Logger::GetInstance().error("JPush config has empty credentials");
    else
        Logger::GetInstance().info("JPush config loaded successfully");
    return cached;
}

std::string compactJson(const Json::Value& value)
{
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    return Json::writeString(writer, value);
}
}

void JPushService::pushToUsers(const std::vector<std::string>& userNames,
                               const std::string& title,
                               const std::string& body,
                               const Json::Value& extras,
                               bool allowSound,
                               bool allowVibration)
{
    if (userNames.empty() || body.empty()) return;
    const auto config = loadConfig();
    if (!config.valid()) return;
    std::vector<PushDeviceRegistrationModel> devices;
    try
    {
        devices = PushDeviceRegistrationDao().getBackgroundDevices(userNames);
    }
    catch (const std::exception& error)
    {
        Logger::GetInstance().error(
            std::string("JPush device lookup failed: ") + error.what());
        return;
    }
    if (devices.empty()) return;

    Json::Value registrationIds(Json::arrayValue);
    bool soundEnabled = false;
    bool vibrationEnabled = false;
    std::unordered_set<std::string> uniqueIds;
    for (const auto& device : devices)
    {
        if (!device.isBannerEnabled()) continue;
        if (uniqueIds.insert(device.getRegistrationId()).second)
            registrationIds.append(device.getRegistrationId());
        soundEnabled = soundEnabled ||
            (allowSound && device.isSoundEnabled());
        vibrationEnabled = vibrationEnabled ||
            (allowVibration && device.isVibrationEnabled());
    }
    if (registrationIds.empty()) return;

    Json::Value payload;
    payload["platform"].append("android");
    payload["audience"]["registration_id"] = registrationIds;
    auto& android = payload["notification"]["android"];
    android["alert"] = body;
    android["title"] = title;
    android["builder_id"] = 1;
    android["priority"] = 1;
    android["alert_type"] = (soundEnabled ? 1 : 0) +
        (vibrationEnabled ? 2 : 0);
    if (extras.isObject()) android["extras"] = extras;
    payload["options"]["apns_production"] = false;

    auto request = drogon::HttpRequest::newHttpJsonRequest(payload);
    request->setMethod(drogon::Post);
    request->setPath("/v3/push");
    request->addHeader(
        "Authorization",
        "Basic " + drogon::utils::base64Encode(
            config.appKey + ":" + config.masterSecret));
    auto client = drogon::HttpClient::newHttpClient("https://api.jpush.cn");
    client->sendRequest(request,
        [client](drogon::ReqResult result,
                 const drogon::HttpResponsePtr& response) {
            if (result != drogon::ReqResult::Ok || !response ||
                response->statusCode() < 200 || response->statusCode() >= 300)
            {
                const int status = response
                    ? static_cast<int>(response->statusCode()) : 0;
                Logger::GetInstance().error(
                    "JPush request failed, httpStatus=" +
                    std::to_string(status));
            }
        },
        10.0);
}
