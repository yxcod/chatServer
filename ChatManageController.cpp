#include "ChatManageController.h"
#include "FriendRelationDao.h"
#include "HeartbeatManager.h"
#include <algorithm>
#include <filesystem>
#include <unordered_set>

// 真正的定义（只出现一次）
std::unordered_map<std::string, WebSocketConnectionPtr> onlineUsers;
std::mutex connMutex;
ChatWSServer* ChatWSServer::instance_ = nullptr;

namespace
{
std::string jsonString(const Json::Value& value);
int jsonInt(const Json::Value& value);

struct PrivacyMessageState
{
    std::string messageKey;
    Json::Value messageId;
    std::string senderId;
    std::unordered_set<std::string> recipients;
    std::unordered_set<std::string> readers;
    bool isGroup{false};
    int readDelaySeconds{10};
    std::string mediaOwnerId;
    std::string mediaName;
    int messageType{1};
};

std::unordered_map<std::string, PrivacyMessageState> privacyMessages;
std::mutex privacyMessagesMutex;

int clampSeconds(const Json::Value& value, int fallback, int minimum, int maximum)
{
    const int parsed = jsonInt(value);
    return std::max(minimum, std::min(maximum, parsed > 0 ? parsed : fallback));
}

std::string messageKey(const Json::Value& value)
{
    if (value.isString()) return value.asString();
    if (value.isUInt64()) return std::to_string(value.asUInt64());
    if (value.isInt64()) return std::to_string(value.asInt64());
    if (value.isUInt()) return std::to_string(value.asUInt());
    if (value.isInt()) return std::to_string(value.asInt());
    return {};
}

bool isOnline(const std::string& userId)
{
    std::lock_guard<std::mutex> lock(connMutex);
    const auto it = onlineUsers.find(userId);
    return it != onlineUsers.end() && it->second && it->second->connected();
}

void sendToUsers(const std::unordered_set<std::string>& userIds,
                 const Json::Value& event)
{
    std::vector<WebSocketConnectionPtr> recipients;
    {
        std::lock_guard<std::mutex> lock(connMutex);
        for (const auto& userId : userIds)
        {
            const auto it = onlineUsers.find(userId);
            if (it != onlineUsers.end() && it->second && it->second->connected())
            {
                recipients.push_back(it->second);
            }
        }
    }
    const std::string payload = jsonString(event);
    for (const auto& recipient : recipients) recipient->send(payload);
}

void sendPrivacyDestroy(const PrivacyMessageState& state,
                        const std::unordered_set<std::string>& userIds,
                        const std::string& reason)
{
    Json::Value event;
    event["type"] = "privacyMessageDestroy";
    event["msgId"] = state.messageId;
    event["privacyMode"] = true;
    event["reason"] = reason;
    sendToUsers(userIds, event);
}

bool safePathSegment(const std::string& value)
{
    return !value.empty() && value != "." && value != ".." &&
        value.find('/') == std::string::npos &&
        value.find('\\') == std::string::npos &&
        value.find(':') == std::string::npos;
}

void removePrivacyMedia(const PrivacyMessageState& state)
{
    if (!safePathSegment(state.mediaOwnerId) ||
        !safePathSegment(state.mediaName)) return;
    std::filesystem::path root;
    switch (state.messageType)
    {
    case 2: root = "./imageData"; break;
    case 3: root = "./audioData"; break;
    case 4: root = "./videoData"; break;
    case 5: root = "./fileData"; break;
    default: return;
    }
    std::error_code ignored;
    std::filesystem::remove(
        root / state.mediaOwnerId / state.mediaName, ignored);
}

void expireUnreadPrivacyMessage(const std::string& key)
{
    PrivacyMessageState state;
    {
        std::lock_guard<std::mutex> lock(privacyMessagesMutex);
        const auto it = privacyMessages.find(key);
        if (it == privacyMessages.end()) return;
        state = it->second;
        privacyMessages.erase(it);
    }
    auto holders = state.recipients;
    holders.insert(state.senderId);
    sendPrivacyDestroy(state, holders, "unread_timeout");
    removePrivacyMedia(state);
}

void registerPrivacyMessage(const Json::Value& jsonMsg,
                            const std::string& senderId,
                            const std::unordered_set<std::string>& recipients,
                            bool isGroup)
{
    PrivacyMessageState state;
    state.messageKey = messageKey(jsonMsg["msgId"]);
    state.messageId = jsonMsg["msgId"];
    state.senderId = senderId;
    state.recipients = recipients;
    state.isGroup = isGroup;
    state.readDelaySeconds = clampSeconds(
        jsonMsg["privacyReadDelaySeconds"], 10, 5, 60);
    state.messageType = jsonInt(jsonMsg["msgType"]);
    state.mediaOwnerId = senderId;
    state.mediaName = jsonMsg["msgContent"].asString();
    if (state.messageType == 3 || state.messageType == 5)
    {
        Json::Value payload;
        Json::CharReaderBuilder builder;
        std::string errors;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        const auto content = state.mediaName;
        if (reader->parse(content.data(), content.data() + content.size(),
                          &payload, &errors))
        {
            state.mediaOwnerId = payload.get("ownerId", senderId).asString();
            state.mediaName = payload.get(
                state.messageType == 3 ? "audioName" : "storedName", "").asString();
        }
    }
    const int unreadDelay = clampSeconds(
        jsonMsg["privacyUnreadDelaySeconds"], 180, 60, 300);
    if (state.messageKey.empty()) return;
    {
        std::lock_guard<std::mutex> lock(privacyMessagesMutex);
        privacyMessages[state.messageKey] = state;
    }
    app().getLoop()->runAfter(unreadDelay, [key = state.messageKey]() {
        expireUnreadPrivacyMessage(key);
    });
}

bool markPrivacyMessageRead(const Json::Value& jsonMsg,
                            const std::string& reader)
{
    const std::string key = messageKey(jsonMsg["msgId"]);
    PrivacyMessageState state;
    bool allRead = false;
    bool eraseState = false;
    {
        std::lock_guard<std::mutex> lock(privacyMessagesMutex);
        const auto it = privacyMessages.find(key);
        if (it == privacyMessages.end() ||
            it->second.recipients.count(reader) == 0) return false;
        if (!it->second.readers.insert(reader).second) return true;
        state = it->second;
        allRead = state.readers.size() == state.recipients.size();
        eraseState = !state.isGroup || allRead;
        if (eraseState) privacyMessages.erase(it);
    }

    Json::Value readEvent;
    readEvent["type"] = "privacyMessageRead";
    readEvent["msgId"] = state.messageId;
    readEvent["reader"] = reader;
    readEvent["privacyMode"] = true;
    readEvent["destroyAfterSeconds"] = state.readDelaySeconds;
    sendToUsers({state.senderId, reader}, readEvent);

    app().getLoop()->runAfter(state.readDelaySeconds,
        [state, reader, allRead]() {
            if (state.isGroup)
            {
                sendPrivacyDestroy(state, {reader}, "read_timeout");
            }
            else
            {
                sendPrivacyDestroy(
                    state, {state.senderId, reader}, "read_timeout");
            }
            if (!state.isGroup || allRead) removePrivacyMedia(state);
        });

    if (state.isGroup && allRead)
    {
        sendPrivacyDestroy(state, {state.senderId}, "all_read");
    }
    return true;
}

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

void ChatWSServer::notifyGroupHistoryDeleted(
    const std::vector<std::string>& memberIds,
    uint64_t groupId,
    const std::string& operatorId)
{
    Json::Value event;
    event["type"] = "groupChatHistoryDeleted";
    event["groupId"] = Json::UInt64(groupId);
    event["operatorId"] = operatorId;
    event["deletedAt"] = Json::UInt64(Logger::GetInstance().getcurrentTime());
    event["message"] = "群主或管理员已删除当前群聊的全部聊天记录";
    const std::string payload = jsonString(event);

    std::vector<WebSocketConnectionPtr> recipients;
    {
        std::lock_guard<std::mutex> lock(connMutex);
        for (const auto& memberId : memberIds)
        {
            // 发起删除的群主由 HTTP 页面流程负责清理和退出，避免重复处理。
            if (memberId == operatorId) continue;
            const auto it = onlineUsers.find(memberId);
            if (it != onlineUsers.end() && it->second && it->second->connected())
            {
                recipients.push_back(it->second);
            }
        }
    }
    for (const auto& recipient : recipients)
    {
        recipient->send(payload);
    }
}

void ChatWSServer::notifyGroupMembersRemoved(
    const std::vector<std::string>& removedUserIds,
    uint64_t groupId,
    const std::string& operatorId)
{
    Json::Value event;
    event["type"] = "groupMemberRemoved";
    event["groupId"] = Json::UInt64(groupId);
    event["operatorId"] = operatorId;
    event["removedAt"] = Json::UInt64(Logger::GetInstance().getcurrentTime());
    event["message"] = "您已被移出该群聊";
    const std::string payload = jsonString(event);

    std::vector<WebSocketConnectionPtr> recipients;
    {
        std::lock_guard<std::mutex> lock(connMutex);
        for (const auto& userId : removedUserIds)
        {
            if (userId == operatorId) continue;
            const auto it = onlineUsers.find(userId);
            if (it != onlineUsers.end() && it->second && it->second->connected())
            {
                recipients.push_back(it->second);
            }
        }
    }
    for (const auto& recipient : recipients) recipient->send(payload);
}

void ChatWSServer::notifyGroupMemberRoleUpdated(
    const std::vector<std::string>& memberIds,
    uint64_t groupId,
    const std::string& targetUserId,
    uint8_t role,
    const std::string& operatorId)
{
    Json::Value event;
    event["type"] = "groupMemberRoleUpdated";
    event["groupId"] = Json::UInt64(groupId);
    event["userName"] = targetUserId;
    event["role"] = role;
    event["operatorId"] = operatorId;
    const std::string payload = jsonString(event);

    std::vector<WebSocketConnectionPtr> recipients;
    {
        std::lock_guard<std::mutex> lock(connMutex);
        for (const auto& memberId : memberIds)
        {
            const auto it = onlineUsers.find(memberId);
            if (it != onlineUsers.end() && it->second && it->second->connected())
            {
                recipients.push_back(it->second);
            }
        }
    }
    for (const auto& recipient : recipients) recipient->send(payload);
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
        if (jsonMsg.get("privacyMode", false).asBool())
        {
            const std::string receiveId = jsonMsg["receiveId"].asString();
            if (receiveId.empty() || !isOnline(receiveId))
            {
                conn->send(chatService.messageFailed(
                    jsonMsg, "privacy recipient is offline"));
                return;
            }
            jsonMsg["sendTime"] = Json::UInt64(
                Logger::GetInstance().getcurrentTime());
            jsonMsg["privacyMode"] = true;
            registerPrivacyMessage(jsonMsg, authenticatedUser, {receiveId}, false);

            Json::Value forward = jsonMsg;
            forward["type"] = "message";
            forward["msgStatus"] = 1;
            sendToUsers({receiveId}, forward);

            Json::Value acknowledgement;
            acknowledgement["type"] = "delivery_ack";
            acknowledgement["msgId"] = jsonMsg["msgId"];
            acknowledgement["status"] = "sent";
            acknowledgement["privacyMode"] = true;
            conn->send(jsonString(acknowledgement));
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
        const bool privacyReadHandled = markPrivacyMessageRead(
            jsonMsg, jsonMsg["sender"].asString());
        if (jsonMsg.get("privacyMode", false).asBool() || privacyReadHandled)
        {
            return;
        }
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
		if (jsonMsg.get("privacyMode", false).asBool())
		{
			if (std::find(userIds.begin(), userIds.end(), sender) == userIds.end())
			{
				Json::Value failure = jsonMsg;
				failure["type"] = "groupChatCallback";
				failure["code"] = 101;
				failure["clientMsgId"] = jsonMsg["msgId"];
				failure["error"] = "sender is not an active group member";
				conn->send(jsonString(failure));
				return;
			}
			std::unordered_set<std::string> onlineRecipients;
			{
				std::lock_guard<std::mutex> lock(connMutex);
				for (const auto& member : userIds)
				{
					if (member == sender) continue;
					const auto it = onlineUsers.find(member);
					if (it != onlineUsers.end() && it->second && it->second->connected())
						onlineRecipients.insert(member);
				}
			}
			jsonMsg["sendTime"] = Json::UInt64(
				Logger::GetInstance().getcurrentTime());
			jsonMsg["privacyMode"] = true;
			if (!onlineRecipients.empty())
				registerPrivacyMessage(jsonMsg, sender, onlineRecipients, true);
			Json::Value forward = jsonMsg;
			forward["type"] = "groupChat";
			forward["code"] = 100;
			sendToUsers(onlineRecipients, forward);
			Json::Value acknowledgement = forward;
			acknowledgement["type"] = "groupChatCallback";
			acknowledgement["clientMsgId"] = jsonMsg["msgId"];
			conn->send(jsonString(acknowledgement));
			if (onlineRecipients.empty())
			{
				PrivacyMessageState emptyState;
				emptyState.messageKey = messageKey(jsonMsg["msgId"]);
				emptyState.messageId = jsonMsg["msgId"];
				emptyState.senderId = sender;
				emptyState.isGroup = true;
				sendPrivacyDestroy(
					emptyState, {sender}, "no_online_recipients");
			}
			return;
		}
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
			const bool privacyReadHandled = markPrivacyMessageRead(jsonMsg, reader);
			if (jsonMsg.get("privacyMode", false).asBool() || privacyReadHandled)
			{
				return;
			}
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
