#include "LoginModel.h"


LoginModel::LoginModel()
{
	account = "";
	password = "";
	isBan = 0;
	time_t now = time(nullptr);
	std::tm localnow{};
	localtime_s(&localnow, &now);
	std::memcpy(&this->registerTime, &localnow, sizeof(std::tm));

}
LoginModel::LoginModel(const std::string& accountVaue, const std::string& passwordVaue, const std::int32_t& state, const std::tm& time)
{
	account = accountVaue;
	password = passwordVaue;
	isBan = state;
	registerTime = time;
}
;
std::string LoginModel::getAccount() const
{
	return account;
}

std::string LoginModel::getPassword() const
{
	return password;
}

std::int32_t LoginModel::getAccountState() const
{
	return isBan;
}

std::tm LoginModel::getRegisterTime() const
{
	return registerTime;
}

void LoginModel::setAccount(const std::string& accountVaue)
{
	account = accountVaue;
}

void LoginModel::setPasswor(const std::string& passwordVaue)
{
	password = passwordVaue;
}

void LoginModel::setAccountState(const std::int32_t& state)
{
	isBan = state;
}

void LoginModel::setRegisterTime(const std::tm& time)
{
	registerTime = time;
}
