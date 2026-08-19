#pragma once
#include <string>
#include <ctime>  // 用于时间类型（数据库 DATETIME 对应 C++ tm 结构体）
#include <sstream>
#include <iomanip>
class LoginModel
{
public:
	LoginModel();

	LoginModel(const std::string& accountVaue, const std::string& passwordVaue, const std::int32_t& state,
		const std::tm& time);

	//获取用户名
	std::string getAccount() const;
	//获取密码
	std::string getPassword()const;
	//获取账号状态 0 正常 1封禁
	std::int32_t getAccountState()const;
	//获取注册时间
	std::tm getRegisterTime()const;

	void setAccount(const std::string& accountVaue);
	void setPasswor(const std::string& passwordVaue);
	void setAccountState(const std::int32_t& state);
	void setRegisterTime(const std::tm& time);

private:
	std::string account;
	std::string password;
	std::int32_t isBan;
	std::tm registerTime;
};

