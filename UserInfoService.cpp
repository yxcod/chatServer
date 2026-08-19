#include "UserInfoService.h"
#include "ChatService.h"
#include <unordered_map>
#include <mutex>
#include "HeartbeatManager.h"

Json::Value UserInfoService::getUserAllInfo(const std::string& userId)
{
	
	UserInfoDao userInfoDao;
	FriendRelationDao friendRelation;
	UserInfo userinfo = userInfoDao.getUserinfo(userId);
	Json::Value jsonObj;
	if(userinfo.getUserAccount().empty())
	{
		jsonObj["code"] = 101;
		return jsonObj;
	}
	jsonObj["code"] = 100;
	jsonObj["userName"] = userinfo.getUserAccount();
	jsonObj["nickName"] = userinfo.getNickName();
	jsonObj["gender"] = userinfo.getGender();
	jsonObj["avatar"] = userinfo.getAvatar();
	//默认占位18 对齐前端结构
	jsonObj["age"] = 18;
	jsonObj["signature"] = userinfo.getSignature();
	jsonObj["modifyTime"] = userinfo.getModifyTime();
	std::vector<UserInfo> friendList;
	friendRelation.getAllFriendWithUserId(userId, 1, friendList);
	Json::Value FriendListarr(Json::arrayValue);
	for (const auto& friendInfo : friendList)
	{
		Json::Value jsonfriendObj;
		jsonfriendObj["userName"] = friendInfo.getUserAccount();
		jsonfriendObj["nickName"] = friendInfo.getNickName();
		jsonfriendObj["avatar"] = friendInfo.getAvatar();
		jsonfriendObj["gender"] = friendInfo.getGender();
		jsonfriendObj["onlineStatus"] = friendInfo.getState()==1 ? true : false;
		jsonfriendObj["signature"] = friendInfo.getSignature();
		jsonfriendObj["remark"] = friendRelation.getFriendRemark(userId, friendInfo.getUserAccount());
		jsonfriendObj["modifyTime"] = friendInfo.getModifyTime();
		FriendListarr.append(jsonfriendObj);
	}
	jsonObj["friendListData"] = FriendListarr;
	return jsonObj;
	
}

Json::Value UserInfoService::modifyUserInfo(const Json::Value& userInfo)
{
	Json::Value jsonObj;
	UserInfoDao userInfoDao;
	UserInfo userinfo;
	std::string userId = userInfo["userName"].asString();
	userinfo.setNickName(userInfo["nickName"].asString());
	userinfo.setSignature(userInfo["signature"].asString());
	userinfo.setAvatar(userInfo["avater"].asString());
	jsonObj["code"] = 101;
	if (userInfoDao.updateUserInfo(userId, userinfo) > 0)
	{
		jsonObj["code"] = 100;
	}
	return jsonObj;
}

void UserInfoService::handleHeartbeat(const std::string& userName)
{
	if (userName.empty())
	{
		return;
	}
	HeartbeatManager::GetInstance().handleHeartbeat(userName);
}

