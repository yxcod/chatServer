#pragma once
#include <drogon/HttpController.h>
#include <openssl/md5.h>
#include <string>
#include "FriendRelationService.h"
#include "ChatManageController.h"
#include "JwtTokenUtil.h"
using namespace drogon;
using namespace drogon::orm;

class FriendController : public HttpController<FriendController> {
public:
	METHOD_LIST_BEGIN
	ADD_METHOD_TO(FriendController::sendFriendApply, "/api/friend/friendApply", Post);
	ADD_METHOD_TO(FriendController::getFriendApply, "/api/friend/requests", Post);
	ADD_METHOD_TO(FriendController::updateFriendApplyState, "/api/friend/handleRequest", Post);
	ADD_METHOD_TO(FriendController::deleteFriend, "/api/friend/delete", Post);
	ADD_METHOD_TO(FriendController::updateFriendRemark, "/api/friend/updateRemark", Post);
	ADD_METHOD_TO(FriendController::getRecentAgreedFriendApply, "/api/friend/recentAgreedRequests", Post);
	METHOD_LIST_END
	// 发送好友申请
		void sendFriendApply(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
	{
		std::cout << "registerUser\n";
		auto json = req->getJsonObject();
		Json::Value response_data;
		if (!json || !json->isMember("fromUserId") || !json->isMember("toUserId") || !json->isMember("applyMsg")) {
			response_data["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		Logger::GetInstance().debugJson(*json);
		FriendRelationService friendService;
		response_data = friendService.sendFriendApply(*json);
		if (response_data["code"].asInt() == 100 &&
			response_data["request"].isObject())
		{
			ChatWSServer::notifyFriendRequestUpdated(
				{(*json)["fromUserId"].asString(), (*json)["toUserId"].asString()},
				response_data["request"], "created");
		}
		callback(HttpResponse::newHttpJsonResponse(response_data));


	}
	//获取向该用户发起好友请求的列表
	void getFriendApply(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
	{
		auto json = req->getJsonObject();
		Json::Value response_data;
		if (!json || !json->isMember("userName")) {
			response_data["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		Logger::GetInstance().debugJson(*json);
		FriendRelationService friendService;
		callback(HttpResponse::newHttpJsonResponse(friendService.getPendingFriendApplyList(*json)));

	}
	//更新好友关系状态
	void updateFriendApplyState(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
	{
		auto json = req->getJsonObject();
		Json::Value response_data;
		if (!json || !json->isMember("requestId") ||
			!json->isMember("requestResult") || !json->isMember("userName")) {
			response_data["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		Logger::GetInstance().debugJson(*json);
		FriendRelationService friendService;
		response_data = friendService.modifyFriendApplyState(*json);
		if (response_data["code"].asInt() == 100 &&
			response_data["request"].isObject())
		{
			const auto& request = response_data["request"];
			const std::string action = request["status"].asInt() == 1
				? "accepted" : "rejected";
			ChatWSServer::notifyFriendRequestUpdated(
				{request["fromUserId"].asString(), request["toUserId"].asString()},
				request, action);
			if (action == "accepted" && response_data["greeting"].isObject())
			{
				ChatWSServer::notifyAutomaticFriendGreeting(
					request["fromUserId"].asString(),
					response_data["greeting"]);
			}
		}
		callback(HttpResponse::newHttpJsonResponse(response_data));

	}
	//删除好友
	void deleteFriend(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
	{
		auto json = req->getJsonObject();
		Logger::GetInstance().debugJson(*json);
		Json::Value response_data;
		if (!json || !json->isMember("fromUserName") || !json->isMember("toUserName")) {
			response_data["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		FriendRelationService friendService;
		callback(HttpResponse::newHttpJsonResponse(friendService.deleteFriend(*json)));

	}
	//更新好友备注
	void updateFriendRemark(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
	{
		auto json = req->getJsonObject();
		Logger::GetInstance().debugJson(*json);
		Json::Value response_data;
		if (!json || !json->isMember("userName") || !json->isMember("friendUserName") || !json->isMember("remark")) {
			response_data["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		FriendRelationService friendService;
		callback(HttpResponse::newHttpJsonResponse(friendService.updateFriendRemark(*json)));
	}
	//获取最近同意的好友申请记录
	void getRecentAgreedFriendApply(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
	{
		auto json = req->getJsonObject();
		Logger::GetInstance().debugJson(*json);
		Json::Value response_data;
		if (!json || !json->isMember("userName")) {
			response_data["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		FriendRelationService friendService;
		callback(HttpResponse::newHttpJsonResponse(friendService.getRecentAgreedFriendApply(*json)));
	}
};
