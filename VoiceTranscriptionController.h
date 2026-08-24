#pragma once

#include <drogon/HttpController.h>

class VoiceTranscriptionController
    : public drogon::HttpController<VoiceTranscriptionController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(
        VoiceTranscriptionController::transcribe,
        "/api/audio/transcribe",
        drogon::Post);
    METHOD_LIST_END

    void transcribe(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};
