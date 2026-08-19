#pragma once
#include"GroupChatDao.h"
#include"GroupMemberDao.h"
#include "GroupMessageDao.h"
#include "GroupMsgReadDao.h"
#include "userinfoDao.h"
#include "GroupConversationDao.h"
class GroupService
{
public:
	//获取用户所有群组信息
	Json::Value getAllGroups(const Json::Value& groupInfo);
	//获取群的所有成员信息
	Json::Value getGroupMembers(const Json::Value& groupInfo);
	//获取群的聊天记录
	Json::Value getGroupChatRecord(const Json::Value& groupInfo);
	//获取用户的群会话列表
	Json::Value getGroupConversations(const Json::Value& userInfo);
	//创建群聊
	Json::Value createGroup(const Json::Value& groupInfo);
	//群添加用户
	Json::Value addGroupMember(const Json::Value& memberInfo);
	//移除人员出群
	Json::Value minuGroupMember(const Json::Value& memberInfo);
	//更新成员群信息
	Json::Value updateGroupMemberInfo(const Json::Value& memberInfo);
	//更新群信息
	Json::Value updateGroupInfo(const Json::Value& groupInfo);
	//获取群成员ID列表
	std::vector<std::string> getUserIds(const int& groupId);
	//消息转发
	std::string handleGroupMessage(const Json::Value& jsonMsg);
	//确认群消息为已读
	std::string groupMessageRead(const Json::Value& jsonMsg);
};

