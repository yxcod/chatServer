#include "FriendRelationService.h"
#include "FriendRelationModel.h"
#include "FriendRelationDao.h"
#include "UserInfoDao.h"
#include "ChatDao.h"
#include "ChatService.h"
#include <algorithm>
#include <atomic>
#include <cctype>

namespace
{
const char* statusName(FriendRelation::RelationStatus status)
{
	switch (status)
	{
	case FriendRelation::RelationStatus::PENDING: return "pending";
	case FriendRelation::RelationStatus::ACCEPTED: return "accepted";
	case FriendRelation::RelationStatus::REJECTED: return "rejected";
	case FriendRelation::RelationStatus::EXPIRED: return "expired";
	default: return "unavailable";
	}
}

Json::Value requestPayload(const FriendRelation& relation)
{
	Json::Value request;
	if (relation.getId() == 0) return request;
	UserInfoDao userInfoDao;
	const auto fromUser = userInfoDao.getUserinfo(relation.getFromUserId());
	const auto toUser = userInfoDao.getUserinfo(relation.getToUserId());
	request["id"] = Json::UInt64(relation.getId());
	request["fromUserId"] = relation.getFromUserId();
	request["toUserId"] = relation.getToUserId();
	request["fromNickName"] = fromUser.getNickName();
	request["toNickName"] = toUser.getNickName();
	request["applyMsg"] = relation.getApplyMsg();
	request["createTime"] = Json::UInt64(relation.getCreateTime());
	request["updateTime"] = Json::UInt64(relation.getUpdateTime());
	request["status"] = relation.getStatusAsUInt8();
	request["statusName"] = statusName(relation.getStatus());
	return request;
}

bool isDigits(const std::string& value)
{
	return !value.empty() && std::all_of(
		value.begin(), value.end(), [](unsigned char character) {
			return std::isdigit(character) != 0;
		});
}

std::string normalizedNumber(const std::string& value)
{
	const auto first = value.find_first_not_of('0');
	return first == std::string::npos ? "0" : value.substr(first);
}

std::string privateSessionId(const std::string& left, const std::string& right)
{
	bool leftIsGreater = false;
	if (isDigits(left) && isDigits(right))
	{
		const auto normalizedLeft = normalizedNumber(left);
		const auto normalizedRight = normalizedNumber(right);
		leftIsGreater = normalizedLeft.size() != normalizedRight.size()
			? normalizedLeft.size() > normalizedRight.size()
			: normalizedLeft > normalizedRight;
	}
	else
	{
		leftIsGreater = left > right;
	}
	return leftIsGreater ? left + "_" + right : right + "_" + left;
}

uint64_t nextAutomaticMessageId()
{
	static std::atomic<uint64_t> lastMessageId{0};
	const uint64_t base = Logger::GetInstance().getcurrentTime() * 1000ULL;
	auto observed = lastMessageId.load(std::memory_order_relaxed);
	uint64_t candidate = 0;
	do
	{
		candidate = (std::max)(base, observed + 1);
	} while (!lastMessageId.compare_exchange_weak(
		observed, candidate, std::memory_order_relaxed));
	return candidate;
}

Json::Value createAutomaticFriendGreeting(const FriendRelation& relation)
{
	Json::Value greeting;
	const std::string senderId = relation.getToUserId();
	const std::string recipientId = relation.getFromUserId();
	const uint64_t now = Logger::GetInstance().getcurrentTime();
	greeting["msgId"] = Json::UInt64(nextAutomaticMessageId());
	greeting["msgContent"] = u8"我们已经成功添加好友啦!";
	greeting["sendUserId"] = senderId;
	greeting["receiveId"] = recipientId;
	greeting["sendTime"] = Json::UInt64(now);
	greeting["readTime"] = Json::UInt64(0);
	greeting["sessionId"] = privateSessionId(senderId, recipientId);
	greeting["receiveType"] = 1;
	greeting["msgType"] = 1;
	greeting["msgStatus"] = 1;
	greeting["extendInfo"] = "{}";

	ChatService chatService;
	const Json::Value insertResult = chatService.insertChatRecord(greeting);
	if (insertResult["code"].asInt() != 100)
	{
		Logger::GetInstance().error(
			"Failed to persist automatic friend greeting for request " +
			std::to_string(relation.getId()));
		return Json::Value();
	}
	greeting["sendTime"] = insertResult["sendTime"];
	greeting["sessionId"] = insertResult["sessionId"];
	return greeting;
}
}

FriendRelationService::FriendRelationService()
{
}

Json::Value FriendRelationService::sendFriendApply(const Json::Value& jsonValue) const
{
	Json::Value response_data;
	response_data["code"] = 101;
	std::string fromUserId = jsonValue["fromUserId"].asString();
	std::string toUserId = jsonValue["toUserId"].asString();
	std::string applyMsg = jsonValue["applyMsg"].asString();
	if (fromUserId.empty() || toUserId.empty() || fromUserId == toUserId)
	{
		response_data["code"] = 99;
		return response_data;
	}
	UserInfoDao userInfoDao;
	UserInfo userInfo = userInfoDao.getUserinfo(toUserId);
	if (userInfo.getUserAccount() == "")
	{
		return response_data;
	}
	response_data["userId"] = userInfo.getUserAccount();
	response_data["nickname"] = userInfo.getNickName();
	FriendRelation friendRelation;
	FriendRelationDao friendRelationDao;
	if (friendRelationDao.hasAcceptedRelation(fromUserId, toUserId))
	{
		response_data["code"] = 103;
		return response_data;
	}
	friendRelation.setFromUserId(fromUserId);
	friendRelation.setToUserId(toUserId);
	friendRelation.setApplyMsg(applyMsg);
	friendRelation.setCreateTime(Logger::GetInstance().getcurrentTime());
	friendRelation.setUpdateTime(Logger::GetInstance().getcurrentTime());
	friendRelation.setFromRemark("");
	friendRelation.setToRemark("");
	friendRelation.setSource("");
	const int inserted = friendRelationDao.insertFriendApply(friendRelation);
	if (inserted > 0)
	{
		response_data["code"] = 100;
		response_data["request"] = requestPayload(
			friendRelationDao.getDirectedFriendRelation(fromUserId, toUserId));
		return response_data;
	}
	response_data["code"] = inserted == -2 ? 103 : 102;
	return response_data;
}

Json::Value FriendRelationService::getPendingFriendApplyList(const Json::Value& jsonValue) const
{
	std::string userName = jsonValue["userName"].asString();
	Json::Value jsonObj;
	FriendRelationDao friendRelationDao;
	jsonObj["code"] = 100;
	jsonObj["applyFriendList"] = Json::arrayValue;
	std::vector<FriendRelation> friendApplyList =
		friendRelationDao.getFriendApplyListForUser(
			userName, Logger::GetInstance().getcurrentTime());
	Json::Value FriendListArr(Json::arrayValue);
	for (const auto& friendInfo : friendApplyList)
	{
		Json::Value jsonfriendObj = requestPayload(friendInfo);
		const bool incoming = friendInfo.getToUserId() == userName;
		jsonfriendObj["direction"] = incoming ? "incoming" : "outgoing";
		const std::string counterpart = incoming
			? friendInfo.getFromUserId() : friendInfo.getToUserId();
		UserInfo counterpartInfo = UserInfoDao().getUserinfo(counterpart);
		jsonfriendObj["userName"] = counterpart;
		jsonfriendObj["nickName"] = counterpartInfo.getNickName();
		jsonfriendObj["canRespond"] = incoming &&
			friendInfo.getStatus() == FriendRelation::RelationStatus::PENDING;
		FriendListArr.append(jsonfriendObj);
	}
	jsonObj["applyFriendList"] = FriendListArr;
	return jsonObj;
}

Json::Value FriendRelationService::modifyFriendApplyState(const Json::Value& jsonValue) const
{
	Json::Value jsonObj;
	jsonObj["code"] = 101;
	FriendRelationDao friendRelationDao;
	int requestId = jsonValue["requestId"].asInt();
	int requestResult = jsonValue["requestResult"].asInt();
	std::string userName = jsonValue["userName"].asString();
	FriendRelation relation = friendRelationDao.getFriendRelationById(requestId);
	if (relation.getId() == 0 ||
		(relation.getFromUserId() != userName &&
			relation.getToUserId() != userName))
	{
		jsonObj["code"] = 403;
		return jsonObj;
	}
	friendRelationDao.getFriendApplyListForUser(
		userName, Logger::GetInstance().getcurrentTime());
	relation = friendRelationDao.getFriendRelationById(requestId);
	if (requestResult ==
		static_cast<int>(FriendRelation::RelationStatus::HASDEL))
	{
		if (relation.getStatus() != FriendRelation::RelationStatus::EXPIRED)
		{
			jsonObj["code"] = 104;
			jsonObj["request"] = requestPayload(relation);
			return jsonObj;
		}
		jsonObj["code"] = friendRelationDao.deleteExpiredFriendApply(
			requestId, userName) > 0 ? 100 : 101;
		return jsonObj;
	}
	if (relation.getToUserId() != userName)
	{
		jsonObj["code"] = 403;
		return jsonObj;
	}
	if (relation.getStatus() != FriendRelation::RelationStatus::PENDING)
	{
		jsonObj["code"] = 104;
		jsonObj["request"] = requestPayload(relation);
		return jsonObj;
	}
	if (friendRelationDao.updateFriendApplyStatus(requestId, requestResult) > 0)
	{
		const FriendRelation updatedRelation =
			friendRelationDao.getFriendRelationById(requestId);
		jsonObj["code"] = 100;
		jsonObj["request"] = requestPayload(updatedRelation);
		if (updatedRelation.getStatus() ==
			FriendRelation::RelationStatus::ACCEPTED)
		{
			const Json::Value greeting =
				createAutomaticFriendGreeting(updatedRelation);
			if (greeting.isObject()) jsonObj["greeting"] = greeting;
		}
	}
	return jsonObj;
}

Json::Value FriendRelationService::deleteFriend(const Json::Value& jsonValue) const
{
	Json::Value jsonObj;
	jsonObj["code"] = 101;
	FriendRelationDao friendRelationDao;
	std::string fromUserName = jsonValue["fromUserName"].asString();
	std::string toUserName = jsonValue["toUserName"].asString();
	std::string sessionId = jsonValue["sessionId"].asString();
	//更新好友关系状态为删除
	if (friendRelationDao.deleteFriendRelation(fromUserName, toUserName) > 0)
	{

		//删除会话表
		ChatDao chatDao;
		//删除会话记录
		chatDao.deleteConversationByConvId(sessionId);
		//删除会话下的聊天记录
		chatDao.deleteChatRecordsBetweenUsers(sessionId);
		jsonObj["code"] = 100;
		return jsonObj;
	}
	return jsonObj;
}

Json::Value FriendRelationService::updateFriendRemark(const Json::Value& jsonValue) const
{
	Json::Value jsonObj;
	jsonObj["code"] = 101;
	FriendRelationDao friendRelationDao;
	std::string fromUserName = jsonValue["userName"].asString();
	std::string toUserName = jsonValue["friendUserName"].asString();
	std::string remarContent = jsonValue["remark"].asString();
	//更新好友备注
	if (friendRelationDao.updateFriendRemark(fromUserName, toUserName, remarContent) > 0)
	{
		jsonObj["code"] = 100;
		return jsonObj;
	}
	return jsonObj;
}

Json::Value FriendRelationService::getRecentAgreedFriendApply(const Json::Value& jsonValue) const
{
	Json::Value jsonObj;
	jsonObj["code"] = 101;
	FriendRelationDao friendRelationDao;
	std::string fromUserName = jsonValue["userName"].asString();
	std::vector<FriendRelation> friendApplyList = friendRelationDao.getRecentFriendApplyByUser(fromUserName, Logger::GetInstance().getcurrentTime());
	Json::Value FriendListArr(Json::arrayValue);
	jsonObj["recentFriendsList"] = FriendListArr;
	for (const auto& friendInfo : friendApplyList)
	{
		jsonObj["code"] = 100;
		const std::string counterpart = friendInfo.getFromUserId() == fromUserName
			? friendInfo.getToUserId() : friendInfo.getFromUserId();
		UserInfoDao userInfoDao;
		UserInfo userInfo = userInfoDao.getUserinfo(counterpart);
		Json::Value jsonfriendObj;
		jsonfriendObj["userName"] = counterpart;
		jsonfriendObj["addTime"] = Json::UInt64(friendInfo.getUpdateTime());
		jsonfriendObj["nickName"] = userInfo.getNickName();
		//这里用ToUserId来代替发起用户的昵称
		jsonfriendObj["remarks"] = userInfo.getNickName();
		FriendListArr.append(jsonfriendObj);
	}
	jsonObj["recentFriendsList"] = FriendListArr;
	return jsonObj;
}
