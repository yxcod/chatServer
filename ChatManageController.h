#pragma once

#include <drogon/HttpController.h>
#include <drogon/WebSocketController.h>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <json/json.h>
#include "ChatService.h"
#include "JwtTokenUtil.h"
#include "UserInfoService.h"
#include "GroupService.h"
using namespace drogon;

// 全局存储在线用户的WebSocket连接（key: userName/userId）
// 这里只声明，定义在 ChatManageController.cpp 中
extern std::unordered_map<std::string, WebSocketConnectionPtr> onlineUsers;
extern std::mutex connMutex;

class ChatWSServer : public WebSocketController<ChatWSServer>
{
public:
    WS_PATH_LIST_BEGIN
        WS_PATH_ADD("/api/chat");
    WS_PATH_LIST_END

    // 提供获取实例的静态函数
    static ChatWSServer* GetInstance();

    // 按用户关闭 WebSocket 连接
    void closeConnectionByUser(const std::string& userName);

    // 通知在线群成员：群主或管理员已删除该群全部聊天记录。
    static void notifyGroupHistoryDeleted(
        const std::vector<std::string>& memberIds,
        uint64_t groupId,
        const std::string& operatorId);

    static void notifyGroupMembersRemoved(
        const std::vector<std::string>& removedUserIds,
        uint64_t groupId,
        const std::string& operatorId);

    static void notifyGroupMemberRoleUpdated(
        const std::vector<std::string>& memberIds,
        uint64_t groupId,
        const std::string& targetUserId,
        uint8_t role,
        const std::string& operatorId);

    static void notifyGroupMemberMuteUpdated(
        const std::vector<std::string>& memberIds,
        uint64_t groupId,
        const std::string& targetUserId,
        bool muted,
        const std::string& operatorId);

    static void notifyGroupSystemMessages(
        const std::vector<std::string>& memberIds,
        const Json::Value& messages);

    static void notifyFriendRequestUpdated(
        const std::vector<std::string>& userIds,
        const Json::Value& request,
        const std::string& action);

    static void notifyAutomaticFriendGreeting(
        const std::string& recipientId,
        const Json::Value& greeting);

    static void notifyMomentInteraction(
        const std::string& recipientId,
        const Json::Value& notification);

    // WebSocketController 接口实现
    void handleNewConnection(const HttpRequestPtr& req,
        const WebSocketConnectionPtr& conn) override;

    void handleNewMessage(const WebSocketConnectionPtr& conn,
        std::string&& message,
        const WebSocketMessageType& type) override;

    void handleConnectionClosed(const WebSocketConnectionPtr& conn) override;
    

private:
    void broadcastPresence(const std::string& userName, bool isOnline);

    static ChatWSServer* instance_;
};
