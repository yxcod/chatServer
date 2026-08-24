#include "CrashHandler.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <thread>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <DbgHelp.h>
#include <process.h>
#pragma comment(lib, "Dbghelp.lib")
#endif

namespace
{
std::filesystem::path gLogDirectory;
std::atomic_flag gWritingCrash = ATOMIC_FLAG_INIT;
std::mutex gInstallMutex;

std::string timestampForFile()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t value = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &value);
#else
    localtime_r(&value, &local);
#endif
    std::ostringstream out;
    out << std::put_time(&local, "%Y%m%d-%H%M%S");
    return out.str();
}

std::string timestampForLog()
{
    const auto now = std::chrono::system_clock::now();
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    const std::time_t value = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &value);
#else
    localtime_r(&value, &local);
#endif
    std::ostringstream out;
    out << std::put_time(&local, "%Y-%m-%d %H:%M:%S") << '.'
        << std::setfill('0') << std::setw(3) << milliseconds.count();
    return out.str();
}

std::filesystem::path crashDirectory()
{
    return gLogDirectory / "crashes";
}

void writeHandlerStatus()
{
    const auto statusPath = crashDirectory() / "crash-handler-ready.log";
    std::ofstream out(statusPath, std::ios::out | std::ios::trunc);
    out << "ChatServer crash handler is active\n";
    out << "installed_at: " << timestampForLog() << '\n';
    out << "crash_directory: " << crashDirectory().u8string() << '\n';
    out << "note: crash-*.log and crash-*.dmp are created only after a fatal error\n";
}

std::filesystem::path createCrashPath(const char* extension)
{
#ifdef _WIN32
    const auto processId = static_cast<unsigned long>(GetCurrentProcessId());
    const auto threadId = static_cast<unsigned long>(GetCurrentThreadId());
#else
    const auto processId = 0UL;
    const auto threadId = static_cast<unsigned long>(
        std::hash<std::thread::id>{}(std::this_thread::get_id()));
#endif
    std::ostringstream name;
    name << "crash-" << timestampForFile() << "-p" << processId
        << "-t" << threadId << extension;
    return crashDirectory() / name.str();
}

void writeBuildInformation(std::ostream& out)
{
    out << "timestamp: " << timestampForLog() << '\n';
    out << "build: " << __DATE__ << ' ' << __TIME__ << '\n';
#ifdef _DEBUG
    out << "configuration: Debug\n";
#else
    out << "configuration: Release\n";
#endif
#if defined(_M_X64) || defined(__x86_64__)
    out << "architecture: x64\n";
#elif defined(_M_IX86) || defined(__i386__)
    out << "architecture: x86\n";
#elif defined(_M_ARM64) || defined(__aarch64__)
    out << "architecture: arm64\n";
#else
    out << "architecture: unknown\n";
#endif
#ifdef _WIN32
    out << "process_id: " << GetCurrentProcessId() << '\n';
    out << "thread_id: " << GetCurrentThreadId() << '\n';
#endif
}

#ifdef _WIN32
const char* exceptionName(DWORD code)
{
    switch (code)
    {
    case EXCEPTION_ACCESS_VIOLATION: return "EXCEPTION_ACCESS_VIOLATION";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_BREAKPOINT: return "EXCEPTION_BREAKPOINT";
    case EXCEPTION_DATATYPE_MISALIGNMENT: return "EXCEPTION_DATATYPE_MISALIGNMENT";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_ILLEGAL_INSTRUCTION: return "EXCEPTION_ILLEGAL_INSTRUCTION";
    case EXCEPTION_IN_PAGE_ERROR: return "EXCEPTION_IN_PAGE_ERROR";
    case EXCEPTION_INT_DIVIDE_BY_ZERO: return "EXCEPTION_INT_DIVIDE_BY_ZERO";
    case EXCEPTION_STACK_OVERFLOW: return "EXCEPTION_STACK_OVERFLOW";
    default: return "UNKNOWN_EXCEPTION";
    }
}

void writeRegisters(std::ostream& out, const CONTEXT& context)
{
    out << std::hex << std::setfill('0');
#ifdef _M_X64
    out << "registers:\n"
        << "  RIP=0x" << std::setw(16) << context.Rip
        << " RSP=0x" << std::setw(16) << context.Rsp
        << " RBP=0x" << std::setw(16) << context.Rbp << '\n'
        << "  RAX=0x" << std::setw(16) << context.Rax
        << " RBX=0x" << std::setw(16) << context.Rbx
        << " RCX=0x" << std::setw(16) << context.Rcx
        << " RDX=0x" << std::setw(16) << context.Rdx << '\n';
#elif defined(_M_IX86)
    out << "registers:\n"
        << "  EIP=0x" << std::setw(8) << context.Eip
        << " ESP=0x" << std::setw(8) << context.Esp
        << " EBP=0x" << std::setw(8) << context.Ebp << '\n';
#endif
    out << std::dec;
}

void writeStackTrace(std::ostream& out, CONTEXT context)
{
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    if (!SymInitialize(process, nullptr, TRUE))
    {
        out << "stack_trace_error: SymInitialize failed, win32="
            << GetLastError() << '\n';
        return;
    }

    STACKFRAME64 frame{};
    DWORD machineType = 0;
#ifdef _M_X64
    machineType = IMAGE_FILE_MACHINE_AMD64;
    frame.AddrPC.Offset = context.Rip;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrStack.Offset = context.Rsp;
#elif defined(_M_IX86)
    machineType = IMAGE_FILE_MACHINE_I386;
    frame.AddrPC.Offset = context.Eip;
    frame.AddrFrame.Offset = context.Ebp;
    frame.AddrStack.Offset = context.Esp;
#else
    out << "stack_trace_error: unsupported Windows architecture\n";
    SymCleanup(process);
    return;
#endif
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Mode = AddrModeFlat;

    out << "stack_trace:\n";
    for (unsigned int index = 0; index < 64; ++index)
    {
        if (frame.AddrPC.Offset == 0)
        {
            break;
        }

        const DWORD64 address = frame.AddrPC.Offset;
        char symbolStorage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME]{};
        auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolStorage);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;
        DWORD64 displacement = 0;

        out << "  [" << std::setw(2) << std::setfill('0') << index << "] 0x"
            << std::hex << address << std::dec;
        const DWORD64 moduleBase = SymGetModuleBase64(process, address);
        if (moduleBase != 0)
        {
            char modulePath[MAX_PATH]{};
            if (GetModuleFileNameA(reinterpret_cast<HMODULE>(moduleBase),
                                   modulePath, MAX_PATH) > 0)
            {
                out << ' ' << std::filesystem::path(modulePath).filename().string()
                    << '!';
            }
        }
        if (SymFromAddr(process, address, &displacement, symbol))
        {
            out << ' ' << symbol->Name << "+0x" << std::hex
                << displacement << std::dec;
        }

        IMAGEHLP_LINE64 line{};
        line.SizeOfStruct = sizeof(line);
        DWORD lineDisplacement = 0;
        if (SymGetLineFromAddr64(process, address, &lineDisplacement, &line))
        {
            out << " (" << line.FileName << ':' << line.LineNumber << ')';
        }
        out << '\n';

        if (!StackWalk64(machineType, process, thread, &frame, &context,
                         nullptr, SymFunctionTableAccess64,
                         SymGetModuleBase64, nullptr))
        {
            break;
        }
    }
    SymCleanup(process);
}

bool writeMiniDump(EXCEPTION_POINTERS* exceptionPointers,
                   const std::filesystem::path& dumpPath)
{
    HANDLE file = CreateFileW(dumpPath.c_str(), GENERIC_WRITE, 0, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    MINIDUMP_EXCEPTION_INFORMATION information{};
    information.ThreadId = GetCurrentThreadId();
    information.ExceptionPointers = exceptionPointers;
    information.ClientPointers = FALSE;
    const auto dumpType = static_cast<MINIDUMP_TYPE>(
        MiniDumpWithThreadInfo | MiniDumpWithIndirectlyReferencedMemory);
    const BOOL written = MiniDumpWriteDump(
        GetCurrentProcess(), GetCurrentProcessId(), file, dumpType,
        exceptionPointers ? &information : nullptr, nullptr, nullptr);
    CloseHandle(file);
    return written == TRUE;
}

LONG WINAPI unhandledExceptionHandler(EXCEPTION_POINTERS* exceptionPointers)
{
    if (gWritingCrash.test_and_set())
    {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    try
    {
        std::filesystem::create_directories(crashDirectory());
        const auto logPath = createCrashPath(".log");
        auto dumpPath = logPath;
        dumpPath.replace_extension(".dmp");
        std::ofstream out(logPath, std::ios::out | std::ios::trunc);
        out << "ChatServer crash report\n";
        writeBuildInformation(out);
        if (exceptionPointers && exceptionPointers->ExceptionRecord)
        {
            const auto& record = *exceptionPointers->ExceptionRecord;
            out << "reason: unhandled structured exception\n";
            out << "exception_name: " << exceptionName(record.ExceptionCode) << '\n';
            out << "exception_code: 0x" << std::hex << record.ExceptionCode
                << std::dec << '\n';
            out << "exception_address: 0x" << std::hex
                << reinterpret_cast<std::uintptr_t>(record.ExceptionAddress)
                << std::dec << '\n';
            if (record.ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
                record.NumberParameters >= 2)
            {
                const auto operation = record.ExceptionInformation[0];
                out << "access_operation: "
                    << (operation == 0 ? "read" : operation == 1 ? "write" : "execute")
                    << '\n';
                out << "access_address: 0x" << std::hex
                    << record.ExceptionInformation[1] << std::dec << '\n';
            }
        }
        if (exceptionPointers && exceptionPointers->ContextRecord)
        {
            writeRegisters(out, *exceptionPointers->ContextRecord);
            writeStackTrace(out, *exceptionPointers->ContextRecord);
        }
        out << "minidump_path: " << dumpPath.string() << '\n';
        out << "minidump_written: "
            << (writeMiniDump(exceptionPointers, dumpPath) ? "true" : "false")
            << '\n';
        out.flush();
    }
    catch (...)
    {
        // Never throw from an OS exception handler.
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

void writeSimpleCrashReport(const std::string& reason) noexcept
{
    if (gWritingCrash.test_and_set())
    {
        return;
    }
    try
    {
        std::filesystem::create_directories(crashDirectory());
        const auto logPath = createCrashPath(".log");
        std::ofstream out(logPath, std::ios::out | std::ios::trunc);
        out << "ChatServer fatal report\n";
        writeBuildInformation(out);
        out << "reason: " << reason << '\n';
#ifdef _WIN32
        CONTEXT context{};
        RtlCaptureContext(&context);
        writeStackTrace(out, context);
        auto dumpPath = logPath;
        dumpPath.replace_extension(".dmp");
        out << "minidump_path: " << dumpPath.string() << '\n';
        out << "minidump_written: "
            << (writeMiniDump(nullptr, dumpPath) ? "true" : "false") << '\n';
#endif
        out.flush();
    }
    catch (...)
    {
    }
}

void terminateHandler() noexcept
{
    std::string reason = "std::terminate called";
    if (const auto exception = std::current_exception())
    {
        try
        {
            std::rethrow_exception(exception);
        }
        catch (const std::exception& e)
        {
            reason += std::string(": ") + e.what();
        }
        catch (...)
        {
            reason += ": non-standard exception";
        }
    }
    writeSimpleCrashReport(reason);
    std::_Exit(EXIT_FAILURE);
}

void signalHandler(int signal) noexcept
{
    writeSimpleCrashReport("fatal signal: " + std::to_string(signal));
    std::_Exit(128 + signal);
}
}

std::filesystem::path CrashHandler::defaultLogDirectory()
{
#ifdef _WIN32
    std::wstring executablePath(32768, L'\0');
    const DWORD size = GetModuleFileNameW(
        nullptr, executablePath.data(), static_cast<DWORD>(executablePath.size()));
    if (size > 0 && size < executablePath.size())
    {
        executablePath.resize(size);
        return std::filesystem::path(executablePath).parent_path() / "logs";
    }
#endif
    return std::filesystem::absolute(std::filesystem::current_path() / "logs");
}

void CrashHandler::install(const std::filesystem::path& logDirectory)
{
    std::lock_guard<std::mutex> lock(gInstallMutex);
    std::set_terminate(terminateHandler);
    gLogDirectory = logDirectory.empty() ? defaultLogDirectory()
                                         : std::filesystem::absolute(logDirectory);
    gWritingCrash.clear();
    std::signal(SIGABRT, signalHandler);
#ifndef _WIN32
    std::signal(SIGFPE, signalHandler);
    std::signal(SIGILL, signalHandler);
    std::signal(SIGSEGV, signalHandler);
#endif
#ifdef _WIN32
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX |
                 SEM_NOOPENFILEERRORBOX);
    SetUnhandledExceptionFilter(unhandledExceptionHandler);
#endif
    std::filesystem::create_directories(crashDirectory());
    writeHandlerStatus();
}

void CrashHandler::recordFatalError(const std::string& reason) noexcept
{
    writeSimpleCrashReport(reason);
}
