#include "ChatManageController.h"
#include "FriendRelationDao.h"
#include "HeartbeatManager.h"

// 真正的定义（只出现一次）
std::unordered_map<std::string, WebSocketConnectionPtr> onlineUsers;
std::mutex connMutex;
ChatWSServer* ChatWSServer::instance_ = nullptr;

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
        UserInfoService userInfoService;
        userInfoService.handleHeartbeat(jsonMsg["userName"].asString());
        return;
    }
    // 正常聊天
    else if (msgType == "chat")
    {
        ChatService chatService;
        Json::Value dbResult = chatService.insertChatRecord(jsonMsg);
        if (dbResult["code"].asInt() != 100)
        {
            return;
        }

        std::string receiveId = jsonMsg["receiveId"].asString();
        {
            std::lock_guard<std::mutex> lock(connMutex);
            auto it = onlineUsers.find(receiveId);
            if (it != onlineUsers.end() && it->second && it->second->connected())
            {
                it->second->send(chatService.handleMessage(jsonMsg));
            }

            std::string sendUserId = jsonMsg["sendUserId"].asString();
            auto itSender = onlineUsers.find(sendUserId);
            if (itSender != onlineUsers.end() && itSender->second && itSender->second->connected())
            {
                itSender->second->send(chatService.messageDelivered(jsonMsg));
            }
        }
    }
    // 消息已读回执
    else if (msgType == "chatCallback")
    {
        ChatService chatService;
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
		GroupService groupService;
        std::string sender = jsonMsg["sendUserId"].asString();
		int groupId = jsonMsg["receiveId"].asInt();
		std::vector<std::string> userIds = groupService.getUserIds(groupId);
		std::string forwardStr = groupService.handleGroupMessage(jsonMsg);
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
        return;

    }
	// 群消息已读回执
    else if (msgType == "groupChatCallback")
    {
        GroupService groupService;
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
