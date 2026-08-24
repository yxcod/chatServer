#pragma once
#include "Logger.h"
#include "UserInfoDao.h"
#include "FriendRelationDao.h"
class ChatService
{
public:
	//获取指定用户的未删除的会话列表
	Json::Value getConversions(const Json::Value& userInfo);
	//更新会话信息
	Json::Value updateConversion(const Json::Value& conversionInfo);
	//插入聊天记录
	Json::Value insertChatRecord(const Json::Value& chatRecord);
	//消息转发
	std::string handleMessage(const Json::Value& jsonMsg);
	//确认消息为已读
	std::string messageRead(const Json::Value& jsonMsg);
	//确认信息为送达
	std::string messageDelivered(const Json::Value& jsonMsg);
	std::string messageFailed(const Json::Value& jsonMsg, const std::string& reason);
	//获取指定用户的所有未读记录
	Json::Value getunReadMessage(const Json::Value& jsonMsg);
	//获取当前用户在指定会话中仍可见的最近聊天记录
	Json::Value getRecentChatRecords(const Json::Value& jsonMsg);
	//单向隐藏私聊记录；双方删除的交集才从数据库物理删除
	Json::Value deletePrivateChatHistory(const Json::Value& jsonMsg);
	//处理视频通话请求
	std::string handleVideoCallRequest(const Json::Value& jsonMsg);
	//处理视频通话同意请求
	std::string handleVideoCallAccept(const Json::Value& jsonMsg);
	//处理视频通话拒绝请求
	std::string handleVideoCallReject(const Json::Value& jsonMsg);
	//处理视频通话结束请求
	std::string handleVideoCallEnd(const Json::Value& jsonMsg);

};
