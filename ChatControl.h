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
	ADD_METHOD_TO(ChatController::deletePrivateChatHistory, "/api/chat/deleteChatHistory", Post);
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

	void deletePrivateChatHistory(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
	{
		auto json = req->getJsonObject();
		Json::Value response_data;
		if (!json || !json->isMember("userName") ||
			!json->isMember("peerUserName") ||
			!json->isMember("conversationId"))
		{
			response_data["code"] = 99;
			response_data["msg"] = "invalid request body";
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		const auto authenticatedUser = getAuthenticatedUser(req);
		if (!authenticatedUser || *authenticatedUser != (*json)["userName"].asString())
		{
			response_data["code"] = 401;
			response_data["msg"] = "unauthorized";
			auto response = HttpResponse::newHttpJsonResponse(response_data);
			response->setStatusCode(k401Unauthorized);
			callback(response);
			return;
		}
		Logger::GetInstance().debugJson(*json);
		ChatService chatService;
		callback(HttpResponse::newHttpJsonResponse(
			chatService.deletePrivateChatHistory(*json)));
	}

private:
	static std::optional<std::string> getAuthenticatedUser(const HttpRequestPtr& req)
	{
		static JwtTokenUtil tokenUtil(
			"c9bb708f526d420ea88d83cd316d662921646869efaf425eb150ab99d20f48bc");
		auto token = tokenUtil.extractBearerToken(req);
		if (!token || !tokenUtil.verifyToken(*token)) return std::nullopt;
		const auto payload = tokenUtil.parsePayload(*token);
		const auto user = payload.find("userId");
		if (user == payload.end() || user->second.empty()) return std::nullopt;
		return user->second;
	}
};
