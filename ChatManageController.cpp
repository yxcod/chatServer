#include "ChatManageController.h"
#include "FriendRelationDao.h"
#include "HeartbeatManager.h"

// 真正的定义（只出现一次）
std::unordered_map<std::string, WebSocketConnectionPtr> onlineUsers;
std::mutex connMutex;
ChatWSServer* ChatWSServer::instance_ = nullptr;

namespace
{
std::string connectedUserName(const WebSocketConnectionPtr& conn)
{
    std::lock_guard<std::mutex> lock(connMutex);
    for (const auto& entry : onlineUsers)
    {
        if (entry.second == conn) return entry.first;
    }
    return {};
}

std::string jsonString(const Json::Value& value)
{
    Json::StreamWriterBuilder builder;
    return Json::writeString(builder, value);
}

int jsonInt(const Json::Value& value)
{
    if (value.isInt() || value.isUInt() || value.isInt64() || value.isUInt64())
    {
        return value.asInt();
    }
    if (value.isString())
    {
        try
        {
            return std::stoi(value.asString());
        }
        catch (...)
        {
        }
    }
    return 0;
}
}

ChatWSServer* ChatWSServer::GetInstance()
{
    return instance_;
}

void ChatWSServer::closeConnectionByUser(const std::string& userName)
{
    WebSocketConnectionPtr connection;
    {
        std::lock_guard<std::mutex> lock(connMutex);
        auto it = onlineUsers.find(userName);
        if (it != onlineUsers.end())
        {
            connection = it->second;
            onlineUsers.erase(it);
        }
    }
    if (!connection) return;
    if (connection->connected()) connection->shutdown();
    broadcastPresence(userName, false);
}

void ChatWSServer::handleNewConnection(const HttpRequestPtr& req,
    const WebSocketConnectionPtr& conn)
{
    auto userName = req->getParameter("userName");
    if (userName.empty())
    {
        conn->shutdown();
        return;
    }

    // 记录当前实例指针
    instance_ = this;

    {
        std::lock_guard<std::mutex> lock(connMutex);
        onlineUsers[userName] = conn;
    }

    UserInfoService().handleHeartbeat(userName);
    broadcastPresence(userName, true);
}

void ChatWSServer::handleNewMessage(const WebSocketConnectionPtr& conn,
    std::string&& message,
    const WebSocketMessageType& type)
{
    if (type != WebSocketMessageType::Text)
    {
        return;
    }

    Json::Value jsonMsg;
    Json::CharReaderBuilder builder;
    std::string errs;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    if (!reader->parse(message.data(), message.data() + message.size(), &jsonMsg, &errs))
    {
        return;
    }

    const std::string msgType = jsonMsg["type"].asString();

    // 心跳处理
    if (msgType == "ping")
    {
        const std::string authenticatedUser = connectedUserName(conn);
        if (authenticatedUser.empty() ||
            authenticatedUser != jsonMsg["userName"].asString()) return;
        UserInfoService userInfoService;
        userInfoService.handleHeartbeat(authenticatedUser);
        return;
    }
    // 正常聊天
    else if (msgType == "chat")
    {
        ChatService chatService;
        const std::string authenticatedUser = connectedUserName(conn);
        if (authenticatedUser.empty() ||
            authenticatedUser != jsonMsg["sendUserId"].asString())
        {
            conn->send(chatService.messageFailed(jsonMsg, "invalid sender"));
            return;
        }
        Json::Value dbResult = chatService.insertChatRecord(jsonMsg);
        if (dbResult["code"].asInt() != 100)
        {
            conn->send(chatService.messageFailed(
                jsonMsg,
                dbResult.get("error", "message persistence failed").asString()));
            return;
        }
		jsonMsg["sessionId"] = dbResult["sessionId"];
		jsonMsg["sendTime"] = dbResult["sendTime"];

        std::string receiveId = jsonMsg["receiveId"].asString();
        {
            std::lock_guard<std::mutex> lock(connMutex);
            auto it = onlineUsers.find(receiveId);
            if (it != onlineUsers.end() && it->second && it->second->connected())
            {
                it->second->send(chatService.handleMessage(jsonMsg));
            }

        }
		conn->send(chatService.messageDelivered(jsonMsg));
    }
    // 消息已读回执
    else if (msgType == "chatCallback")
    {
        ChatService chatService;
		if (connectedUserName(conn) != jsonMsg["sender"].asString()) return;
        std::string receiveId = jsonMsg["receiveId"].asString();
        std::lock_guard<std::mutex> lock(connMutex);
        auto it = onlineUsers.find(receiveId);
        std::string forward = chatService.messageRead(jsonMsg);
		//若对方在线则转发已读回执 不在线则只处理数据库中已读标记
        if (it != onlineUsers.end() && it->second && it->second->connected())
        {
            it->second->send(forward);
        }
    }
	// 视频通话邀请
    else if (msgType == "videoCallInvite")
    {
        ChatService chatService;
		//被邀请方
        std::string receiveId = jsonMsg["receiver"].asString();
        std::lock_guard<std::mutex> lock(connMutex);
        auto it = onlineUsers.find(receiveId);
        std::string forward = chatService.handleVideoCallRequest(jsonMsg);
       
        if (it != onlineUsers.end() && it->second && it->second->connected())
        {
            it->second->send(forward);
        }
        


    }
    // 视频通话同意
    else if (msgType == "videoCallAccept")
    {
        ChatService chatService;
		//主动发起视频通话方
        std::string receiveId = jsonMsg["receiver"].asString();
        std::lock_guard<std::mutex> lock(connMutex);
        auto it = onlineUsers.find(receiveId);
        std::string forward = chatService.handleVideoCallAccept(jsonMsg);
        if (it != onlineUsers.end() && it->second && it->second->connected())
        {
            it->second->send(forward);
        }


    }
    // 视频通话拒绝
    else if (msgType == "videoCallReject")
    {
        ChatService chatService;
        //主动发起视频通话方
        std::string receiveId = jsonMsg["receiver"].asString();
        std::lock_guard<std::mutex> lock(connMutex);
        auto it = onlineUsers.find(receiveId);
        std::string forward = chatService.handleVideoCallReject(jsonMsg);
        if (it != onlineUsers.end() && it->second && it->second->connected())
        {
            it->second->send(forward);
        }


    }
	//视频通话挂断
    else if (msgType == "videoCallHangup")
    {
        ChatService chatService;
        //对方
        std::string receiveId = jsonMsg["receiver"].asString();
        std::lock_guard<std::mutex> lock(connMutex);
        auto it = onlineUsers.find(receiveId);
        std::string forward = chatService.handleVideoCallEnd(jsonMsg);
        if (it != onlineUsers.end() && it->second && it->second->connected())
        {
            it->second->send(forward);
        }
    }
    // 群聊消息处理
    else if (msgType == "groupChat")
    {
		try
		{
			GroupService groupService;
			std::string sender = jsonMsg["sendUserId"].asString();
		if (connectedUserName(conn) != sender)
		{
			Json::Value failure = jsonMsg;
			failure["type"] = "groupChatCallback";
			failure["code"] = 101;
			failure["clientMsgId"] = jsonMsg["msgId"];
			failure["error"] = "invalid sender";
			conn->send(jsonString(failure));
			return;
		}
		int groupId = jsonMsg["receiveId"].asInt();
		std::vector<std::string> userIds = groupService.getUserIds(groupId);
		Json::Value result = groupService.handleGroupMessage(jsonMsg);
		if (result["code"].asInt() != 100)
		{
			result["type"] = "groupChatCallback";
			conn->send(jsonString(result));
			return;
		}
		std::string forwardStr = jsonString(result);
        std::lock_guard<std::mutex> lock(connMutex);
        for (const auto& member : userIds)
        {
			//判断用户是否在群内且在线则转发消息
            auto it = onlineUsers.find(member);
			// 不发送给自己
            if (sender == member)
            {
                continue;
            }
			//用户在线则转发
            if (it != onlineUsers.end() && it->second && it->second->connected())
            {
                it->second->send(forwardStr);
            }
        }
		Json::Value acknowledgement = result;
		acknowledgement["type"] = "groupChatCallback";
		conn->send(jsonString(acknowledgement));
		}
		catch (const std::exception& e)
		{
			Logger::GetInstance().error(std::string("groupChat failed: ") + e.what());
			Json::Value failure = jsonMsg;
			failure["type"] = "groupChatCallback";
			failure["code"] = 101;
			failure["clientMsgId"] = jsonMsg["msgId"];
			failure["error"] = "group message processing failed";
			if (conn && conn->connected()) conn->send(jsonString(failure));
		}
		catch (...)
		{
			Logger::GetInstance().error("groupChat failed: unknown exception");
		}
        return;

    }
	// 群消息已读回执
    else if (msgType == "groupChatCallback")
    {
		try
		{
			GroupService groupService;
		const std::string reader = jsonMsg["sender"].asString();
		if (connectedUserName(conn) != reader) return;
		const int groupId = jsonInt(jsonMsg.get(
			"groupId", jsonMsg.get("sessionId", jsonMsg.get("receiveId", 0))));
		const auto memberIds = groupService.getUserIds(groupId);
		if (std::find(memberIds.begin(), memberIds.end(), reader) == memberIds.end()) return;
        //反馈给receiveId 他的消息已读
        std::string receiveId = jsonMsg["receiveId"].asString();
		std::string forward = groupService.groupMessageRead(jsonMsg);
        std::lock_guard<std::mutex> lock(connMutex);
		auto it = onlineUsers.find(receiveId);
		//若对方在线则转发已读回执 不在线则只处理数据库中已读标记
        if (it != onlineUsers.end() && it->second && it->second->connected())
        {
            it->second->send(forward);
        }
		}
		catch (const std::exception& e)
		{
			Logger::GetInstance().error(std::string("groupChatCallback failed: ") + e.what());
		}
		catch (...)
		{
			Logger::GetInstance().error("groupChatCallback failed: unknown exception");
		}
        return;

    }
    // 用户进入群聊后按消息水位批量清除未读，并广播给在线群成员。
    else if (msgType == "groupChatRead")
    {
		try
		{
			const std::string reader = jsonMsg["reader"].asString();
			if (reader.empty() || connectedUserName(conn) != reader) return;
			const int groupId = jsonInt(jsonMsg.get("groupId", jsonMsg["sessionId"]));
			const uint64_t readThroughMsgId =
				jsonMsg["readThroughMsgId"].asUInt64();
			if (groupId <= 0 || readThroughMsgId == 0) return;

			GroupService groupService;
			const auto memberIds = groupService.getUserIds(groupId);
			if (std::find(memberIds.begin(), memberIds.end(), reader) ==
				memberIds.end()) return;

			const Json::Value result = groupService.markGroupMessagesRead(
				reader, static_cast<uint64_t>(groupId), readThroughMsgId);
			const std::string forward = jsonString(result);
			std::vector<WebSocketConnectionPtr> recipients;
			{
				std::lock_guard<std::mutex> lock(connMutex);
				for (const auto& member : memberIds)
				{
					const auto it = onlineUsers.find(member);
					if (it != onlineUsers.end() && it->second &&
						it->second->connected())
					{
						recipients.push_back(it->second);
					}
				}
			}
			for (const auto& recipient : recipients)
			{
				recipient->send(forward);
			}
		}
		catch (const std::exception& e)
		{
			Logger::GetInstance().error(std::string("groupChatRead failed: ") + e.what());
		}
		catch (...)
		{
			Logger::GetInstance().error("groupChatRead failed: unknown exception");
		}
		return;
    }
   
    
	
}

void ChatWSServer::handleConnectionClosed(const WebSocketConnectionPtr& conn)
{
    std::string disconnectedUser;
    {
        std::lock_guard<std::mutex> lock(connMutex);
        for (auto it = onlineUsers.begin(); it != onlineUsers.end(); ++it)
        {
            if (it->second == conn)
            {
                disconnectedUser = it->first;
                onlineUsers.erase(it);
                break;
            }
        }
    }

    // 旧连接可能在同一用户重连后才关闭，此时不能把新会话标记为离线。
    if (disconnectedUser.empty()) return;
    HeartbeatManager::GetInstance().handleDisconnect(disconnectedUser);
    broadcastPresence(disconnectedUser, false);
}

void ChatWSServer::broadcastPresence(const std::string& userName,
                                     bool isOnline)
{
    std::vector<UserInfo> friends;
    FriendRelationDao().getAllFriendWithUserId(userName, 1, friends);

    std::vector<WebSocketConnectionPtr> recipients;
    {
        std::lock_guard<std::mutex> lock(connMutex);
        recipients.reserve(friends.size());
        for (const auto& friendInfo : friends)
        {
            const auto it = onlineUsers.find(friendInfo.getUserAccount());
            if (it != onlineUsers.end() && it->second &&
                it->second->connected())
            {
                recipients.push_back(it->second);
            }
        }
    }

    Json::Value event(Json::objectValue);
    event["type"] = "presence";
    event["userName"] = userName;
    event["onlineStatus"] = isOnline;
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    const std::string payload = Json::writeString(writer, event);
    for (const auto& recipient : recipients)
    {
        recipient->send(payload);
    }
}
