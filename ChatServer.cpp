// ChatServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。

#include "Logger.h"
#include "UserController.h"
#include "FilesController.h"
#include"ChatControl.h"
#include"ChatManageController.h"
#include"FriendController.h"
#include"GroupController.h"
using namespace drogon;


int main() {
	
	//初始化MYSQl
	Logger::GetInstance().initMysSql();
	//初始化服务器
	Logger::GetInstance().initService();
	return -1;
	
}
