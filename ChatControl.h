#pragma once
#include <drogon/HttpController.h>
#include <openssl/md5.h>
#include <string>
#include "ChatService.h"
#include "JwtTokenUtil.h"
using namespace drogon;
using namespace drogon::orm;

class ChatController : public HttpController<ChatController> {
public:
	METHOD_LIST_BEGIN
	ADD_METHOD_TO(ChatController::getConversions, "/api/chat/conversation", Post);
	ADD_METHOD_TO(ChatController::getUnReadMessage, "/api/chat/unReadMessage", Post);
	ADD_METHOD_TO(ChatController::getRecentChatRecords, "/api/chat/chatMessages", Post);
	METHOD_LIST_END
		// 获取指定用户的所有未被移除的会话列表
		void getConversions(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
		{
			auto json = req->getJsonObject();
			Logger::GetInstance().debugJson(*json);
			Json::Value response_data;
			if (!json || !json->isMember("userName")) {
				response_data["code"] = 99;
				callback(HttpResponse::newHttpJsonResponse(response_data));
				return;
			}
			ChatService chatService;
			callback(HttpResponse::newHttpJsonResponse(chatService.getConversions(*json)));


		}
	void getUnReadMessage(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
	{
		auto json = req->getJsonObject();
		Logger::GetInstance().debugJson(*json);
		Json::Value response_data;
		if (!json || !json->isMember("userName")) {
			response_data["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		ChatService chatService;
		callback(HttpResponse::newHttpJsonResponse(chatService.getunReadMessage(*json)));

	}

	void getRecentChatRecords(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
	{
		auto json = req->getJsonObject();
		Logger::GetInstance().debugJson(*json);
		Json::Value response_data;
		if (!json || !json->isMember("conversationId") || !json->isMember("limit")) {
			response_data["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		ChatService chatService;
		callback(HttpResponse::newHttpJsonResponse(chatService.getRecentChatRecords(*json)));
	}


};