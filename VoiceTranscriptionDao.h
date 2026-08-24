#pragma once

#include <optional>
#include <string>

#include "VoiceTranscriptionModel.h"

class VoiceTranscriptionDao {
public:
    std::optional<VoiceTranscriptionModel> find(
        const std::string& audioOwnerId,
        const std::string& audioName,
        const std::string& audioSha256,
        const std::string& engineType) const;

    bool upsert(const VoiceTranscriptionModel& transcription) const;
};
