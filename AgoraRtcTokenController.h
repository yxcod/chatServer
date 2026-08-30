#pragma once
#include <drogon/HttpController.h>

class AgoraRtcTokenController
    : public drogon::HttpController<AgoraRtcTokenController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(AgoraRtcTokenController::issue,
                      "/api/calls/rtc-token", drogon::Post);
    METHOD_LIST_END
    void issue(const drogon::HttpRequestPtr& request,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};
