#include "UserModel.h"
#include <stdexcept>  // 用于抛出非法参数异常（可选）

// 1. 默认构造函数：初始化默认值
UserInfo::UserInfo()// state(2)，默认值为2 SQL语句中为2则表示不写入
	: userId(0), gender(-1), state(2){
	// 初始化注册时间为当前系统时间
	//setCreateTimeNow();
}

// 2. 带参数构造函数：快速初始化所有字段
UserInfo::UserInfo(uint64_t userId, const std::string& userAccount, const std::string& nickName,
	const std::string& avatar, const int& gender, const std::string& region, const std::string& signature,
	const uint64_t& createTime, const int& state, const uint64_t& modifyTime) {
	this->userId = userId;
	this->userAccount = userAccount;
	this->nickName = nickName;
	this->avatar = avatar;
	this->region = region;
	this->signature = signature;
	this->createTime = createTime;
	this->state = state;
	this->modifyTime = modifyTime;
	// 调用setGender确保性别值合法
	setGender(gender);
}

// 3. 拷贝构造函数：深拷贝（字符串字段默认深拷贝，无需额外处理）
UserInfo::UserInfo(const UserInfo& other) {
	this->userId = other.userId;
	this->userAccount = other.userAccount;
	this->nickName = other.nickName;
	this->avatar = other.avatar;
	this->gender = other.gender;
	this->region = other.region;
	this->signature = other.signature;
	this->createTime = other.createTime;
	this->state = other.state;
	this->modifyTime = other.modifyTime;
}

// 4. 赋值运算符重载：深拷贝
UserInfo& UserInfo::operator=(const UserInfo& other) {
	if (this != &other) {  // 避免自赋值
		this->userId = other.userId;
		this->userAccount = other.userAccount;
		this->nickName = other.nickName;
		this->avatar = other.avatar;
		this->gender = other.gender;
		this->region = other.region;
		this->signature = other.signature;
		this->createTime = other.createTime;
		this->state = other.state;
		this->modifyTime = other.modifyTime;
	}
	return *this;
}

// -------------------------- Getter 函数实现 --------------------------
uint64_t UserInfo::getUserId() const {
	return userId;
}

const std::string& UserInfo::getUserAccount() const {
	return userAccount;
}

const std::string& UserInfo::getNickName() const {
	return nickName;
}

const std::string& UserInfo::getAvatar() const {
	return avatar;
}

int UserInfo::getGender() const {
	return gender;
}

const std::string& UserInfo::getRegion() const {
	return region;
}

const std::string& UserInfo::getSignature() const {
	return signature;
}

const uint64_t& UserInfo::getCreateTime() const {
	return createTime;
}

const uint64_t& UserInfo::getModifyTime() const
{
	return modifyTime;
}

const int& UserInfo::getState() const
{
	return state;
}

// -------------------------- Setter 函数实现 --------------------------
void UserInfo::setUserId(uint64_t userId) {
	// 可选：限制userId不能为0（根据业务需求）
	if (userId == 0) {
		throw std::invalid_argument("userId cannot be 0");
	}
	this->userId = userId;
}

void UserInfo::setUserAccount(const std::string& userAccount) {
	// 可选：校验用户名长度（数据库字段VARCHAR(50)，最长50字符）
	if (userAccount.length() > 50) {
		throw std::invalid_argument("userName length cannot exceed 50 characters");
	}
	this->userAccount = userAccount;
}

void UserInfo::setNickName(const std::string& nickName) {
	// 校验昵称长度（数据库VARCHAR(50)）
	if (nickName.length() > 50) {
		throw std::invalid_argument("nickName length cannot exceed 50 characters");
	}
	this->nickName = nickName;
}

void UserInfo::setAvatar(const std::string& avatar) {
	// 校验头像URL长度（数据库VARCHAR(255)）
	if (avatar.length() > 255) {
		throw std::invalid_argument("avatar URL length cannot exceed 255 characters");
	}
	this->avatar = avatar;
}

void UserInfo::setGender(int gender) {
	// 校验性别合法性（仅0=未知，1=男，2=女）
	if (gender < -1 || gender > 2) {
		// 方案1：抛出异常（严格校验）
		throw std::invalid_argument("gender must be 0 (unknown), 1 (male), or 2 (female)");
		// 方案2：默认设为0（宽松校验，避免崩溃）
		// this->gender = 0;
	}
	this->gender = gender;
}

void UserInfo::setRegion(const std::string& region) {
	if (region.length() > 100) {
		throw std::invalid_argument("region length cannot exceed 100 characters");
	}
	this->region = region;
}

void UserInfo::setSignature(const std::string& signature) {
	// 校验个性签名长度（数据库VARCHAR(200)）
	if (signature.length() > 200) {
		throw std::invalid_argument("signature length cannot exceed 200 characters");
	}
	this->signature = signature;
}

void UserInfo::setCreateTime(const std::uint64_t& createTime) {
	this->createTime = createTime;
}

void UserInfo::setModifyTime(const uint64_t& modifyTime)
{
	this->modifyTime = modifyTime;
}

// 便捷方法：设置注册时间为当前系统时间
void UserInfo::setCreateTimeNow() {
	time_t now = time(nullptr);
	std::tm localnow{};
	localtime_s(&localnow, &now);
	std::memcpy(&this->createTime, &localnow, sizeof(std::tm));
	
}

void UserInfo::setState(const uint8_t& state)
{
	this->state = state;
}

// -------------------------- 辅助函数：对象转字符串（调试用）--------------------------
std::string UserInfo::toString() const {

	// 拼接所有字段
	std::stringstream result;
	result << "UserInfo{"
		<< "userId=" << userId
		<< ", userAccount='" << userAccount << '\''
		<< ", nickName='" << nickName << '\''
		<< ", avatar='" << avatar << '\''
		<< ", gender=" << (gender == 0 ? "未知" : (gender == 1 ? "男" : "女"))
		<< ", region='" << region << '\''
		<< ", signature='" << signature << '\''
		<< ", createTime='" << createTime << '\''
		<< '}';
	return result.str();
}
