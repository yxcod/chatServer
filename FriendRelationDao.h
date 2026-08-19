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
	//增加好友申请信息
	int insertFriendApply(const FriendRelation& friendRelation) const;
	//更新好友申请状态 传入唯一ID和状态值
	int updateFriendApplyStatus(const int& relationId, const int& state) const;
	//删除好友关系
	int deleteFriendRelation(const std::string& userId1, const std::string& userId2) const;
	//获取向userid发送好友认证的信息
	std::vector<FriendRelation> getToUseridFriendApplyList(const std::string& userId) const;
	//根据两个用户ID查询好友关系
	FriendRelation getFriendRelation(const std::string& userId1, const std::string& userId2) const;
	//根据两个用户ID查询好友备注
	std::string getFriendRemark(const std::string& userId1, const std::string& userId2) const;
	//根据两个用户ID修改好友备注（无顺序），返回受影响行数
	int updateFriendRemark(const std::string& userId1, const std::string& userId2, const std::string& remark) const;
	//获取某用户最近的同意的好友申请记录（3天内）
	std::vector<FriendRelation> getRecentFriendApplyByUser(const std::string& userName, const uint64_t& nowTs) const;

};

