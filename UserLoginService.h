#pragma once
#include "Logger.h"
#include "EncryptUtil.h"
#include "JwtTokenUtil.h"
#include <filesystem>
class UserLoginService {
public:
	// 构造函数：注入 DAO 层和工具类（依赖注入，便于测试和扩展）
	UserLoginService();

	// 析构函数（默认）
	~UserLoginService() = default;
	//注册
	Json::Value registerUser(const std::string& account, const std::string& password);
	//登录
	Json::Value login(const std::string& account,const std::string& password);
	//修改密码
	Json::Value changePassword(const std::string& account, const std::string& oldPassword, const std::string& newPassword);
	// 忘记密码：前端完成安全码校验后重置密码
	Json::Value resetPassword(const std::string& account, const std::string& newPassword);
	// 在当前路径下的 imageData / videoData / fileData 目录中，
	// 为传入的 name 创建对应子目录（不存在则自动创建）
	void createUserDataDirectories(const std::string& name) const;

private:
	
	

private:

	std::shared_ptr<EncryptUtil> encryptUtil;
	std::shared_ptr<JwtTokenUtil> tokenUtil;
};
