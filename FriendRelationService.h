#pragma once
#include "Logger.h"
#include "FriendRelationDao.h"
class FriendRelationService
{
public:
	FriendRelationService();
	~FriendRelationService() = default;
	//发送好友申请
	Json::Value sendFriendApply(const Json::Value& jsonValue) const;
	//获取对应发送给userId验证的好友列表
	Json::Value getPendingFriendApplyList(const Json::Value& jsonValue) const;
	//更新好友申请状态
	Json::Value modifyFriendApplyState(const Json::Value& jsonValue) const;
	//删除好友
	Json::Value deleteFriend(const Json::Value& jsonValue) const;
	//更新好友备注
	Json::Value updateFriendRemark(const Json::Value& jsonValue) const;
	//获取最近同意的好友申请记录
	Json::Value getRecentAgreedFriendApply(const Json::Value& jsonValue) const;
};

