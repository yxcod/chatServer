#pragma once
#include <drogon/HttpController.h>
#include <openssl/md5.h>
#include <string>
#include "FriendRelationService.h"
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
		Logger::GetInstance().debugJson(*json);
		Json::Value response_data;
		if (!json || !json->isMember("fromUserId") || !json->isMember("toUserId") || !json->isMember("applyMsg")) {
			response_data["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		FriendRelationService friendService;
		callback(HttpResponse::newHttpJsonResponse(friendService.sendFriendApply(*json)));


	}
	//获取向该用户发起好友请求的列表
	void getFriendApply(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
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
		callback(HttpResponse::newHttpJsonResponse(friendService.getPendingFriendApplyList(*json)));

	}
	//更新好友关系状态
	void updateFriendApplyState(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
	{
		auto json = req->getJsonObject();
		Logger::GetInstance().debugJson(*json);
		Json::Value response_data;
		if (!json || !json->isMember("requestId") || !json->isMember("requestResult")) {
			response_data["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		FriendRelationService friendService;
		callback(HttpResponse::newHttpJsonResponse(friendService.modifyFriendApplyState(*json)));

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
