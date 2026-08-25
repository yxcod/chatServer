#pragma once
#include <drogon/HttpController.h>
#include <openssl/md5.h>
#include <string>
#include "UserInfoService.h"
#include "UserLoginService.h"
#include "JwtTokenUtil.h"
using namespace drogon;
using namespace drogon::orm;

class UserController : public HttpController<UserController> {
public:
	METHOD_LIST_BEGIN
	ADD_METHOD_TO(UserController::registerUser, "/api/user/register", Post);
	ADD_METHOD_TO(UserController::login, "/api/user/login", Post);
	ADD_METHOD_TO(UserController::getUserInfo, "/api/user/userInfo", Post);
	ADD_METHOD_TO(UserController::userPasswordChange, "/api/user/passwordModify", Post);
	ADD_METHOD_TO(UserController::userPasswordReset, "/api/user/passwordReset", Post);
	ADD_METHOD_TO(UserController::userInfoChange, "/api/user/userInfoModify", Post);
	METHOD_LIST_END
	// 用户注册
	void registerUser(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) 
	{
		std::cout << "registerUser\n";
		auto json = req->getJsonObject();
		Logger::GetInstance().debugJson(*json);
		Json::Value response_data;
		if (!json || !json->isMember("userName") || !json->isMember("password")) {
			response_data["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		std::string userAccount = (*json)["userName"].asString();
		std::string password = (*json)["password"].asString();
		UserLoginService login;
		callback(HttpResponse::newHttpJsonResponse(login.registerUser(userAccount, password)));
	
		
	}

	// 用户登陆
	void login(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
		auto json = req->getJsonObject();
		Logger::GetInstance().debugJson(*json);
		Json::Value response_data;
		if (!json || !json->isMember("userName") || !json->isMember("password")) {
			response_data["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		std::string userAccount = (*json)["userName"].asString();
		std::string password = (*json)["password"].asString();
		UserLoginService login;
		callback(HttpResponse::newHttpJsonResponse(login.login(userAccount, password)));
	}
	//获取用户信息
	void getUserInfo(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {

		// JwtTokenUtil should be initialized from CHATSERVER_JWT_SECRET.
		//optional<string> token = jwtTokenUtil.extractBearerToken(req);
		//auto resp = HttpResponse::newHttpResponse();
		//if (!token.has_value()) {
		//	// Token 不存在或格式错误，返回 401 未授权
		//	resp->setStatusCode(k403Forbidden);
		//	resp->setBody(R"({"code":-2})");
		//	resp->setContentTypeCode(CT_APPLICATION_JSON);
		//	callback(resp);
		//	return;
		//}

		//// 2. 验证 Token有效性
		//bool tokenValid = jwtTokenUtil.verifyToken(token.value());
		//if (!tokenValid) {
		//	// Token 无效/过期，返回 403 禁止访问
		//	resp->setStatusCode(k400BadRequest);
		//	resp->setBody(R"({"code":-3})");
		//	resp->setContentTypeCode(CT_APPLICATION_JSON);
		//	callback(resp);
		//	return;
		//}

		auto json = req->getJsonObject();
		Logger::GetInstance().debugJson(*json);
		Json::Value response_data;
		if (!json || !json->isMember("userName")) {
			response_data["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		std::string userAccount = (*json)["userName"].asString();
		
		UserInfoService userInfo;
		callback(HttpResponse::newHttpJsonResponse(userInfo.getUserAllInfo(userAccount)));
		Logger::GetInstance().debugJson(userInfo.getUserAllInfo(userAccount));
	}
	//用户修改密码
	void userPasswordChange(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
		auto json = req->getJsonObject();
		Logger::GetInstance().debugJson(*json);
		Json::Value response_data;
		if (!json || !json->isMember("userName") || !json->isMember("oldPassword") || !json->isMember("newPassword")) {
			response_data["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		std::string userAccount = (*json)["userName"].asString();
		std::string oldPassword = (*json)["oldPassword"].asString();
		std::string newPassword = (*json)["newPassword"].asString();
		UserLoginService login;
		callback(HttpResponse::newHttpJsonResponse(login.changePassword(userAccount, oldPassword, newPassword)));
	}
	// 忘记密码重置。安全码按照产品需求仅由前端校验，此接口只接收重置结果。
	void userPasswordReset(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
		auto json = req->getJsonObject();
		Json::Value responseData;
		if (!json || !json->isMember("userName") || !json->isMember("newPassword")) {
			responseData["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(responseData));
			return;
		}

		const std::string userAccount = (*json)["userName"].asString();
		const std::string newPassword = (*json)["newPassword"].asString();
		if (userAccount.empty() || newPassword.length() < 6) {
			responseData["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(responseData));
			return;
		}

		UserLoginService login;
		callback(HttpResponse::newHttpJsonResponse(
			login.resetPassword(userAccount, newPassword)));
	}
	//修改个人信息
	void userInfoChange(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
		auto json = req->getJsonObject();
		Logger::GetInstance().debugJson(*json);
		Json::Value response_data;
		if (!json || !json->isMember("userName")) {
			response_data["code"] = 99;
			callback(HttpResponse::newHttpJsonResponse(response_data));
			return;
		}
		UserInfoService userInfo;
		callback(HttpResponse::newHttpJsonResponse(userInfo.modifyUserInfo(*json)));
	}
};
