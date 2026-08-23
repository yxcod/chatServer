#pragma once

#include <filesystem>
#include <string>

class CrashHandler
{
public:
    CrashHandler() = delete;

    // Installs process-wide handlers before database and network initialization.
    static void install(const std::filesystem::path& logDirectory = {});

    // Returns an absolute logs directory next to the executable on Windows.
    static std::filesystem::path defaultLogDirectory();

    // Records fatal exceptions caught at the main boundary.
    static void recordFatalError(const std::string& reason) noexcept;
};
