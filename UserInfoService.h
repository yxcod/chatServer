#pragma once
#include "Logger.h"
#include "UserInfoDao.h"
#include "FriendRelationDao.h"
#include <json/json.h>

class UserInfoService
{
public:
	//获取个人信息包括好友列表信息 Json::Value
	Json::Value getUserAllInfo(const std::string& userId);
	//修改个人信息
	Json::Value modifyUserInfo(const Json::Value& userInfo);

	// 心跳检测：前端每隔5秒调用一次，传入 userName，更新在线状态
	void handleHeartbeat(const std::string& userName);
	
private:
	//获取备注
};

