#include "FriendRelationService.h"
#include "FriendRelationModel.h"
#include "FriendRelationDao.h"
#include "UserInfoDao.h"
#include "ChatDao.h"
FriendRelationService::FriendRelationService()
{
}

Json::Value FriendRelationService::sendFriendApply(const Json::Value& jsonValue) const
{
	Json::Value response_data;
	//用户不存在
	response_data["code"] = 101;
	std::string fromUserId = jsonValue["fromUserId"].asString();
	std::string toUserId = jsonValue["toUserId"].asString();
	std::string applyMsg = jsonValue["applyMsg"].asString();
	UserInfoDao userInfoDao;
	UserInfo userInfo = userInfoDao.getUserinfo(toUserId);
	if (userInfo.getUserAccount() == "")
	{
		return response_data;
	}
	response_data["userId"] = userInfo.getUserAccount();
	response_data["nickname"] = userInfo.getNickName();
	FriendRelation friendRelation;
	FriendRelationDao friendRelationDao;
	friendRelation.setFromUserId(fromUserId);
	friendRelation.setToUserId(toUserId);
	friendRelation.setApplyMsg(applyMsg);
	friendRelation.setCreateTime(Logger::GetInstance().getcurrentTime());
	friendRelation.setUpdateTime(Logger::GetInstance().getcurrentTime());
	friendRelation.setFromRemark("");
	friendRelation.setToRemark("");
	friendRelation.setSource("");
	if (friendRelationDao.insertFriendApply(friendRelation) > 0)
	{
		response_data["code"] = 100;
		return response_data;
	}
	//好友表插入失败
	response_data["code"] = 102;
	return response_data;
}

Json::Value FriendRelationService::getPendingFriendApplyList(const Json::Value& jsonValue) const
{
	std::string userName = jsonValue["userName"].asString();
	Json::Value jsonObj;
	FriendRelationDao friendRelationDao;
	//表示当前无发送请求
	jsonObj["code"] = 101;
	jsonObj["applyFriendList"] = Json::arrayValue;
	std::vector<FriendRelation> friendApplyList = friendRelationDao.getToUseridFriendApplyList(userName);
	if (friendApplyList.size() == 0)
	{
		return jsonObj;
	}
	jsonObj["code"] = 100;
	Json::Value FriendListArr(Json::arrayValue);
	for (const auto& friendInfo : friendApplyList)
	{
		Json::Value jsonfriendObj;
		jsonfriendObj["id"] = friendInfo.getId();
		jsonfriendObj["fromUserId"] = friendInfo.getFromUserId();
		jsonfriendObj["createTime"] = friendInfo.getCreateTime();
		jsonfriendObj["applyMsg"] = friendInfo.getApplyMsg();
		//这里用ToUserId来代替发起用户的昵称
		jsonfriendObj["nickName"] = friendInfo.getToUserId();
		FriendListArr.append(jsonfriendObj);
	}
	jsonObj["applyFriendList"] = FriendListArr;
	return jsonObj;
}

Json::Value FriendRelationService::modifyFriendApplyState(const Json::Value& jsonValue) const
{
	Json::Value jsonObj;
	jsonObj["code"] = 101;
	FriendRelationDao friendRelationDao;
	int requestId = jsonValue["requestId"].asInt();
	int requestResult = jsonValue["requestResult"].asInt();
	std::string sessionId = jsonValue["sessionId"].asString();
	if (friendRelationDao.updateFriendApplyStatus(requestId, requestResult) > 0)
	{
		
		jsonObj["code"] = 100;
	}
	return jsonObj;
}

Json::Value FriendRelationService::deleteFriend(const Json::Value& jsonValue) const
{
	Json::Value jsonObj;
	jsonObj["code"] = 101;
	FriendRelationDao friendRelationDao;
	std::string fromUserName = jsonValue["fromUserName"].asString();
	std::string toUserName = jsonValue["toUserName"].asString();
	std::string sessionId = jsonValue["sessionId"].asString();
	//更新好友关系状态为删除
	if (friendRelationDao.deleteFriendRelation(fromUserName, toUserName) > 0)
	{

		//删除会话表
		ChatDao chatDao;
		//删除会话记录
		chatDao.deleteConversationByConvId(sessionId);
		//删除会话下的聊天记录
		chatDao.deleteChatRecordsBetweenUsers(sessionId);
		jsonObj["code"] = 100;
		return jsonObj;
	}
	return jsonObj;
}

Json::Value FriendRelationService::updateFriendRemark(const Json::Value& jsonValue) const
{
	Json::Value jsonObj;
	jsonObj["code"] = 101;
	FriendRelationDao friendRelationDao;
	std::string fromUserName = jsonValue["userName"].asString();
	std::string toUserName = jsonValue["friendUserName"].asString();
	std::string remarContent = jsonValue["remark"].asString();
	//更新好友备注
	if (friendRelationDao.updateFriendRemark(fromUserName, toUserName, remarContent) > 0)
	{
		jsonObj["code"] = 100;
		return jsonObj;
	}
	return jsonObj;
}

Json::Value FriendRelationService::getRecentAgreedFriendApply(const Json::Value& jsonValue) const
{
	Json::Value jsonObj;
	jsonObj["code"] = 101;
	FriendRelationDao friendRelationDao;
	std::string fromUserName = jsonValue["userName"].asString();
	std::vector<FriendRelation> friendApplyList = friendRelationDao.getRecentFriendApplyByUser(fromUserName, Logger::GetInstance().getcurrentTime());
	Json::Value FriendListArr(Json::arrayValue);
	if (friendApplyList.size() == 0)
	{
		return jsonObj;
	}
	for (const auto& friendInfo : friendApplyList)
	{
		jsonObj["code"] = 100;
		UserInfoDao userInfoDao;
		UserInfo userInfo = userInfoDao.getUserinfo(friendInfo.getFromUserId());
		Json::Value jsonfriendObj;
		jsonfriendObj["userName"] = friendInfo.getFromUserId();
		jsonfriendObj["addTime"] = friendInfo.getCreateTime();
		jsonfriendObj["nickName"] = userInfo.getNickName();
		//这里用ToUserId来代替发起用户的昵称
		jsonfriendObj["remarks"] = userInfo.getNickName();
		FriendListArr.append(jsonfriendObj);
	}
	jsonObj["recentFriendsList"] = FriendListArr;
	return jsonObj;
}
