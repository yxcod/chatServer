#include "FriendRelationModel.h"
#include <stdexcept>
#include <algorithm>

// 1. 默认构造函数：初始化默认值
FriendRelation::FriendRelation()
	: id(0), fromUserId(""), toUserId(""), status(RelationStatus::PENDING),
	  createTime(0), updateTime(0) {
	
}

// 2. 带参数构造函数
FriendRelation::FriendRelation(uint64_t id, std::string fromUserId, std::string toUserId,
	RelationStatus status, const std::string& fromRemark,
	const std::string& toRemark, const std::string& source,
	const std::string& applyMsg, const uint64_t& createTime,
	const uint64_t& updateTime) {
	this->id = id;
	setFromUserId(fromUserId);
	setToUserId(toUserId);
	this->status = status;
	setFromRemark(fromRemark);
	setToRemark(toRemark);
	setSource(source);
	setApplyMsg(applyMsg);
	this->createTime = createTime;
	this->updateTime = updateTime;
}

// 3. 拷贝构造函数
FriendRelation::FriendRelation(const FriendRelation& other) {
	this->id = other.id;
	this->fromUserId = other.fromUserId;
	this->toUserId = other.toUserId;
	this->status = other.status;
	this->fromRemark = other.fromRemark;
	this->toRemark = other.toRemark;
	this->source = other.source;
	this->applyMsg = other.applyMsg;
	this->createTime = other.createTime;
	this->updateTime = other.updateTime;
}

// 4. 赋值运算符重载
FriendRelation& FriendRelation::operator=(const FriendRelation& other) {
	if (this != &other) {
		this->id = other.id;
		this->fromUserId = other.fromUserId;
		this->toUserId = other.toUserId;
		this->status = other.status;
		this->fromRemark = other.fromRemark;
		this->toRemark = other.toRemark;
		this->source = other.source;
		this->applyMsg = other.applyMsg;
		this->createTime = other.createTime;
		this->updateTime = other.updateTime;
	}
	return *this;
}

// -------------------------- Getter 函数实现 --------------------------
uint64_t FriendRelation::getId() const {
	return id;
}

std::string FriendRelation::getFromUserId() const {
	return fromUserId;
}

std::string FriendRelation::getToUserId() const {
	return toUserId;
}

FriendRelation::RelationStatus FriendRelation::getStatus() const {
	return status;
}

uint8_t FriendRelation::getStatusAsUInt8() const {
	return static_cast<uint8_t>(status);
}

const std::string& FriendRelation::getFromRemark() const {
	return fromRemark;
}

const std::string& FriendRelation::getToRemark() const {
	return toRemark;
}

const std::string& FriendRelation::getSource() const {
	return source;
}

const std::string& FriendRelation::getApplyMsg() const {
	return applyMsg;
}

const uint64_t& FriendRelation::getCreateTime() const {
	return createTime;
}

const uint64_t& FriendRelation::getUpdateTime() const {
	return updateTime;
}

// -------------------------- Setter 函数实现 --------------------------
void FriendRelation::setId(uint64_t id) {
	this->id = id;
}

void FriendRelation::setFromUserId(std::string fromUserId) {
	if (fromUserId == "") {
		throw std::invalid_argument("fromUserId cannot be 0");
	}
	this->fromUserId = fromUserId;
}

void FriendRelation::setToUserId(std::string toUserId) {
	if (toUserId == "") {
		throw std::invalid_argument("toUserId cannot be 0");
	}
	if (toUserId == fromUserId) {
		throw std::invalid_argument("toUserId cannot be the same as fromUserId");
	}
	this->toUserId = toUserId;
}

void FriendRelation::setStatus(RelationStatus status) {
	this->status = status;
	setUpdateTimeNow();  // 状态变更时自动更新时间
}

void FriendRelation::setStatus(uint8_t status) {
	if (status > static_cast<uint8_t>(RelationStatus::HASDEL)) {
		throw std::invalid_argument("invalid status value: " + std::to_string(status));
	}
	this->status = static_cast<RelationStatus>(status);
	setUpdateTimeNow();
}

void FriendRelation::setFromRemark(const std::string& fromRemark) {
	if (fromRemark.length() > 50) {
		throw std::invalid_argument("fromRemark length cannot exceed 50 characters");
	}
	this->fromRemark = fromRemark;
}

void FriendRelation::setToRemark(const std::string& toRemark) {
	if (toRemark.length() > 50) {
		throw std::invalid_argument("toRemark length cannot exceed 50 characters");
	}
	this->toRemark = toRemark;
}

void FriendRelation::setSource(const std::string& source) {
	if (source.length() > 30) {
		throw std::invalid_argument("source length cannot exceed 30 characters");
	}
	this->source = source;
}

void FriendRelation::setApplyMsg(const std::string& applyMsg) {
	if (applyMsg.length() > 200) {
		throw std::invalid_argument("applyMsg length cannot exceed 200 characters");
	}
	this->applyMsg = applyMsg;
}

void FriendRelation::setCreateTime(const uint64_t& createTime) {
	this->createTime = createTime;
}

void FriendRelation::setCreateTimeNow() {
	this->createTime = static_cast<uint64_t>(std::time(nullptr));
}

void FriendRelation::setUpdateTime(const uint64_t& updateTime) {
	this->updateTime = updateTime;
}

void FriendRelation::setUpdateTimeNow() {
	this->updateTime = static_cast<uint64_t>(std::time(nullptr));
}

// 返回当前好友关系状态的中文描述，便于日志及接口展示。
std::string FriendRelation::getStatusDesc() const {
	switch (status) {
	case RelationStatus::PENDING: return u8"\u5F85\u9A8C\u8BC1";
	case RelationStatus::ACCEPTED: return u8"\u5DF2\u901A\u8FC7";
	case RelationStatus::REJECTED: return u8"\u5DF2\u62D2\u7EDD";
	case RelationStatus::BLOCKED: return u8"\u5DF2\u62C9\u9ED1";
	default: return u8"\u672A\u77E5\u72B6\u6001";
	}
}

std::string FriendRelation::toString() const {
	
	std::stringstream result;
	result << "FriendRelation{"
		<< "id=" << id
		<< ", fromUserId=" << fromUserId
		<< ", toUserId=" << toUserId
		<< ", status=" << getStatusDesc() << "(" << static_cast<uint8_t>(status) << ")"
		<< ", fromRemark='" << fromRemark << '\''
		<< ", toRemark='" << toRemark << '\''
		<< ", source='" << source << '\''
		<< ", applyMsg='" << applyMsg << '\''
		<< ", createTime='" << createTime << '\''
		<< ", updateTime='" << updateTime << '\''
		<< '}';
	return result.str();
}
