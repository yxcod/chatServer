#pragma once
#include <string>
#include <ctime>  // 用于时间类型（数据库 DATETIME 对应 C++ tm 结构体）
#include <sstream>
#include <iomanip>
#include "Logger.h"
#include "FriendRelationModel.h"
#include "UserModel.h"
class FriendRelationDao
{
public:
	FriendRelationDao();
	//获取该账号所有对应状态好友 state（0=待验证，1=已通过，2=已拒绝，3=已拉黑）
	void getAllFriendWithUserId(const std::string& userId, const int& state,std::vector <UserInfo> &userInfoList)const;
	// Insert or reuse an application. -2: accepted, -3: reverse pending.
	int insertFriendApply(const FriendRelation& friendRelation) const;
	//更新好友申请状态 传入唯一ID和状态值
	int updateFriendApplyStatus(const int& relationId, const int& state) const;
	// 删除当前用户可见且已经过期的好友申请。
	int deleteExpiredFriendApply(
		uint64_t relationId, const std::string& userId) const;
	//删除好友关系
	int deleteFriendRelation(const std::string& userId1, const std::string& userId2) const;
	// Return incoming and outgoing application records visible to the user.
	std::vector<FriendRelation> getFriendApplyListForUser(
		const std::string& userId, const uint64_t& nowTs) const;
	FriendRelation getFriendRelationById(uint64_t relationId) const;
	FriendRelation getDirectedFriendRelation(
		const std::string& fromUserId, const std::string& toUserId) const;
	//根据两个用户ID查询好友关系
	FriendRelation getFriendRelation(const std::string& userId1, const std::string& userId2) const;
	bool hasAcceptedRelation(const std::string& userId1, const std::string& userId2) const;
	//根据两个用户ID查询好友备注
	std::string getFriendRemark(const std::string& userId1, const std::string& userId2) const;
	//根据两个用户ID修改好友备注（无顺序），返回受影响行数
	int updateFriendRemark(const std::string& userId1, const std::string& userId2, const std::string& remark) const;
	// 获取某用户最近三天内已同意或已拒绝的好友申请记录。
	std::vector<FriendRelation> getRecentFriendApplyByUser(const std::string& userName, const uint64_t& nowTs) const;

};
