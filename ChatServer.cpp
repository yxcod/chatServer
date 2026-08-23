// ChatServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。

#include "Logger.h"
#include "CrashHandler.h"
#include "UserController.h"
#include "FilesController.h"
#include"ChatControl.h"
#include"ChatManageController.h"
#include"FriendController.h"
#include"GroupController.h"
#include "MomentController.h"
#include "GroupResourceController.h"
#include <cstdlib>
using namespace drogon;


int main()
{
    const auto logDirectory = CrashHandler::defaultLogDirectory();
    CrashHandler::install(logDirectory);
    Logger::GetInstance().initLogging(logDirectory.u8string());
    Logger::GetInstance().info("ChatServer process starting");

    try
    {
        Logger::GetInstance().initMysSql();
        Logger::GetInstance().info("Database connection pool initialized");
        Logger::GetInstance().initService();
        Logger::GetInstance().info("ChatServer stopped normally");
        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        const std::string reason = std::string("fatal exception in main: ") + e.what();
        Logger::GetInstance().error(reason);
        CrashHandler::recordFatalError(reason);
    }
    catch (...)
    {
        const std::string reason = "unknown fatal exception in main";
        Logger::GetInstance().error(reason);
        CrashHandler::recordFatalError(reason);
    }
    return EXIT_FAILURE;
}
