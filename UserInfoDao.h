#pragma once
#include <string>
#include <ctime>  // 用于时间类型（数据库 DATETIME 对应 C++ tm 结构体）
#include <sstream>
#include <iomanip>
#include "Logger.h"
#include "UserModel.h"
//userId, userName, nickName, avatar, gender, signature, createTime, state
enum class UserInfoValueType {
	userId,
	userName,
	nickName,
	avatar,
	gender,
	signature,
	createTime,
	state,
	all

};
class UserInfoDao
{
public:
	UserInfoDao();
	//插入用户信息
	int insertUserInfo(const UserInfo& userInfo);
	//根据用户账号获取用户所有信息
	UserInfo getUserinfo(const std::string& userId)const;
	//根据用户账号更改用户指定信息 userName(账号)和创建时间无法修改
	int updateUserInfo(const std::string& userId,const UserInfo& userInfo);
	//根据用户账号获取对应键值
	UserInfo getUserValueWithType(const UserInfoValueType& type, const std::string& userId)const;
	
	
private:
	std::map<UserInfoValueType, std::string> userInfoKey;

};

