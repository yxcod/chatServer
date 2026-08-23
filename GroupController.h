#pragma once
#include <drogon/HttpController.h>
#include <openssl/md5.h>
#include <string>
#include "Logger.h"
#include "JwtTokenUtil.h"
#include "GroupService.h"
using namespace drogon;
using namespace drogon::orm;

class GroupController : public HttpController<GroupController> {
public:
	METHOD_LIST_BEGIN
		ADD_METHOD_TO(GroupController::getAllGroup, "/api/group/getGroups", Post);
		ADD_METHOD_TO(GroupController::getGroupMembers, "/api/group/getGroupMember", Post);
		ADD_METHOD_TO(GroupController::getGroupChatRecord, "/api/group/groupChatRecord", Post);
		ADD_METHOD_TO(GroupController::createGroup, "/api/group/createGroup", Post);
		ADD_METHOD_TO(GroupController::addGroupMember, "/api/group/addGroup", Post);
		ADD_METHOD_TO(GroupController::minueGroupMember, "/api/group/minuGroup", Post);
		ADD_METHOD_TO(GroupController::updateGroupMemberInfo, "/api/group/updateGroupMemberInfo", Post);
		ADD_METHOD_TO(GroupController::updateGroupInfo, "/api/group/updateGroupInfo", Post);
		ADD_METHOD_TO(GroupController::getGroupConversations, "/api/group/groupConversation", Post);
		ADD_METHOD_TO(GroupController::deleteGroupChatHistory, "/api/group/deleteGroupChatHistory", Post);
	
	
	METHOD_LIST_END
	// 获取用户所有群组信息
		void getAllGroup(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
	{
		
		auto json = req->getJsonObject();
		Logger::GetInstance().debugJson(*json);
		Json::Value response_data;
		if (!json || !json->isMember("userName")) {
			response_data["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		GroupService groupService;
		callback(HttpResponse::newHttpJsonResponse(groupService.getAllGroups(*json)));
		

	}
	//获取群的所有成员信息
	void getGroupMembers(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
	{
		auto json = req->getJsonObject();
		Json::Value response_data;
		if (!json || !json->isMember("groupId")) {
			response_data["code"] = 99;
			response_data["msg"] = "invalid request body";
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		Logger::GetInstance().debugJson(*json);
		try {
			GroupService groupService;
			callback(HttpResponse::newHttpJsonResponse(groupService.getGroupMembers(*json)));
		}
		catch (const std::exception& e) {
			Logger::GetInstance().error(std::string("getGroupMembers failed: ") + e.what());
			response_data["code"] = 98;
			response_data["msg"] = "internal server error";
			callback(HttpResponse::newHttpJsonResponse(response_data));
		}
		catch (...) {
			Logger::GetInstance().error("getGroupMembers failed: unknown exception");
			response_data["code"] = 98;
			response_data["msg"] = "internal server error";
			callback(HttpResponse::newHttpJsonResponse(response_data));
		}
	}
	//获取群的聊天记录
	void getGroupChatRecord(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
	{
		auto json = req->getJsonObject();
		Json::Value response_data;
		if (!json || !json->isMember("groupId")) {
			response_data["code"] = 99;
			response_data["msg"] = "invalid request body";
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		Logger::GetInstance().debugJson(*json);
		try {
			GroupService groupService;
			callback(HttpResponse::newHttpJsonResponse(groupService.getGroupChatRecord(*json)));
		}
		catch (const std::exception& e) {
			Logger::GetInstance().error(std::string("getGroupChatRecord failed: ") + e.what());
			response_data["code"] = 98;
			response_data["msg"] = "internal server error";
			callback(HttpResponse::newHttpJsonResponse(response_data));
		}
		catch (...) {
			Logger::GetInstance().error("getGroupChatRecord failed: unknown exception");
			response_data["code"] = 98;
			response_data["msg"] = "internal server error";
			callback(HttpResponse::newHttpJsonResponse(response_data));
		}
	}
	// 群主删除全群聊天记录
	void deleteGroupChatHistory(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
	{
		auto json = req->getJsonObject();
		Json::Value response_data;
		if (!json || !json->isMember("groupId") || !json->isMember("userName"))
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
		GroupService groupService;
		callback(HttpResponse::newHttpJsonResponse(
			groupService.deleteGroupChatHistory(*json)));
	}
	//获取群的会话列表
	void getGroupConversations(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
	{
		auto json = req->getJsonObject();
		Logger::GetInstance().debugJson(*json);
		Json::Value response_data;
		if (!json || !json->isMember("userName")) {
			response_data["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		GroupService groupService;
		callback(HttpResponse::newHttpJsonResponse(groupService.getGroupConversations(*json)));
	}
	//创建群聊
	void createGroup(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
	{
		auto json = req->getJsonObject();
		Logger::GetInstance().debugJson(*json);
		Json::Value response_data;
		if (!json || !json->isMember("createUserName") || !json->isMember("groupId") || !json->isMember("groupName")) {
			response_data["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		GroupService groupService;
		callback(HttpResponse::newHttpJsonResponse(groupService.createGroup(*json)));
	}
	//群添加用户
	void addGroupMember(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
	{
		auto json = req->getJsonObject();
		Logger::GetInstance().debugJson(*json);
		Json::Value response_data;
		if (false/*!json->isMember("groupId")*//* ||
			!json->isMember("userNames")*/) 
		{
			response_data["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		GroupService groupService;
		callback(HttpResponse::newHttpJsonResponse(groupService.addGroupMember(*json)));
	}
	//移除用户出群
	void minueGroupMember(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
	{
		auto json = req->getJsonObject();
		Logger::GetInstance().debugJson(*json);
		Json::Value response_data;
		if (!json || !json->isMember("userNames") || !json->isMember("groupId"))
		{
			response_data["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		GroupService groupService;
		callback(HttpResponse::newHttpJsonResponse(groupService.minuGroupMember(*json)));
	}
	//更新成员群信息
	void updateGroupMemberInfo(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
	{
		auto json = req->getJsonObject();
		Logger::GetInstance().debugJson(*json);
		Json::Value response_data;
		if (!json->isMember("groupId") ||
			!json->isMember("userName")) 
		{
			response_data["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		GroupService groupService;
		callback(HttpResponse::newHttpJsonResponse(groupService.updateGroupMemberInfo(*json)));
	}
	//更新群信息
	void updateGroupInfo(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
	{
		auto json = req->getJsonObject();
		Logger::GetInstance().debugJson(*json);
		Json::Value response_data;
		if (!json->isMember("groupId")) 
		{
			response_data["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		GroupService groupService;
		callback(HttpResponse::newHttpJsonResponse(groupService.updateGroupInfo(*json)));
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
