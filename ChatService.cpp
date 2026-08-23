#include"ChatService.h"
#include"ChatDao.h"
#include <json/json.h>
#include "ChatRecoedModel.h"
#include"ChatDao.h"
Json::Value ChatService::getConversions(const Json::Value& userInfo)
{
    Json::Value result;
    ChatDao dao;
    std::string userId = userInfo["userName"].asString();
    auto convs = dao.getUserAllConversation(userId);
    Json::Value arr(Json::arrayValue);
    for (const auto& c : convs) {
        Json::Value item;
        item["convId"] = c.getConvId();
        item["convType"] = static_cast<int>(c.getConvType());
        item["user1Id"] = c.getUser1Id();
        item["user2Id"] = c.getUser2Id();
        item["groupId"] = c.getGroupId();
        item["lastMsg"] = c.getLastMsg();
        item["lastMsgId"] = c.getLastMsgId();
        item["lastSenderId"] = c.getLastSenderId();
        item["unreadCount"] = c.getUser1UnreadCount();
        if (userId == c.getUser2Id()) {
            item["unreadCount"] = c.getUser2UnreadCount();
        }
        item["updateTime"] = static_cast<Json::UInt64>(c.getUpdateTime());
        item["user2isValid"] = static_cast<int>(c.getUser2isValid());
        item["user1isVaild"] = static_cast<int>(c.getUser1isVaild());
        arr.append(item);
    }

    result["code"] = 100; // success
    result["conversationList"] = arr;
    return result;
}

Json::Value ChatService::updateConversion(const Json::Value& conversionInfo)
{
    Json::Value result;
    ConversationModel conv;

    if (conversionInfo.isMember("convId")) conv.setConvId(conversionInfo["convId"].asString());
    if (conversionInfo.isMember("convType")) conv.setConvType(static_cast<uint8_t>(conversionInfo["convType"].asInt()));
    if (conversionInfo.isMember("user1Id")) conv.setUser1Id(conversionInfo["user1Id"].asString());
    if (conversionInfo.isMember("user2Id")) conv.setUser2Id(conversionInfo["user2Id"].asString());
    if (conversionInfo.isMember("groupId")) conv.setGroupId(conversionInfo["groupId"].asString());
    if (conversionInfo.isMember("lastMsg")) conv.setLastMsg(conversionInfo["lastMsg"].asString());
    if (conversionInfo.isMember("lastMsgId")) conv.setLastMsgId(conversionInfo["lastMsgId"].asString());
    if (conversionInfo.isMember("lastSenderId")) conv.setLastSenderId(conversionInfo["lastSenderId"].asString());
    if (conversionInfo.isMember("user1UnreadCount")) conv.setUser1UnreadCount(conversionInfo["user1UnreadCount"].asInt());
    if (conversionInfo.isMember("user2UnreadCount")) conv.setUser2UnreadCount(conversionInfo["user2UnreadCount"].asInt());
    if (conversionInfo.isMember("updateTime")) conv.setUpdateTime(conversionInfo["updateTime"].asUInt64());
    if (conversionInfo.isMember("user2isValid")) conv.setUser2isValid(static_cast<uint8_t>(conversionInfo["user2isValid"].asInt()));
    if (conversionInfo.isMember("user1isVaild")) conv.setUser1isVaild(static_cast<uint8_t>(conversionInfo["user1isVaild"].asInt()));

    ChatDao dao;
    int affected = dao.updateConversation(conv);
    if (affected > 0) {
        result["code"] = 100;
    }
    else {
        result["code"] = 101;
    }
    return result;
}

Json::Value ChatService::insertChatRecord(const Json::Value& chatRecord)
{
    Json::Value result;
    result["code"] = 101; // 默认失败

    ChatRecord rec;

    // JSON 键名与 ChatRecord 成员变量同名
    if (chatRecord.isMember("id")) {
        rec.setId(chatRecord["id"].asUInt64());
    }
    if (chatRecord.isMember("msgId")) {
        rec.setMsgId(chatRecord["msgId"].asUInt64());
    }
    if (chatRecord.isMember("sessionId")) {
        rec.setSessionId(chatRecord["sessionId"].asString());
    }
    if (chatRecord.isMember("sendUserId")) {
        rec.setSendUserId(chatRecord["sendUserId"].asString());
    }
    if (chatRecord.isMember("receiveType")) {
        // receiveType 存储为数值，转为枚举
        uint8_t rt = static_cast<uint8_t>(chatRecord["receiveType"].asUInt());
        rec.setReceiveType(rt);
    }
    if (chatRecord.isMember("receiveId")) {
        rec.setReceiveId(chatRecord["receiveId"].asString());
    }
    if (chatRecord.isMember("msgType")) {
        uint8_t mt = static_cast<uint8_t>(chatRecord["msgType"].asUInt());
        rec.setMsgType(mt);
    }
    if (chatRecord.isMember("msgContent")) {
        rec.setMsgContent(chatRecord["msgContent"].asString());
    }
    if (chatRecord.isMember("msgStatus")) {
        uint8_t ms = static_cast<uint8_t>(chatRecord["msgStatus"].asUInt());
        rec.setMsgStatus(ms);
    }
    if (chatRecord.isMember("sendTime")) {
        rec.setSendTime(chatRecord["sendTime"].asUInt64());
    }
    if (chatRecord.isMember("readTime")) {
        rec.setReadTime(chatRecord["readTime"].asUInt64());
    }
    if (chatRecord.isMember("extendInfo")) {
        // 直接存字符串，若前端传的是 JSON 字符串即可
        rec.setExtendInfo(chatRecord["extendInfo"].asString());
    }

    if (rec.getMsgId() == 0 || rec.getSessionId().empty() ||
        rec.getSendUserId().empty() || rec.getReceiveId().empty())
    {
        result["error"] = "invalid private message";
        return result;
    }
    if (rec.getSendTime() == 0)
    {
        rec.setSendTime(Logger::GetInstance().getcurrentTime());
    }
	rec.setMsgStatus(static_cast<uint8_t>(ChatRecord::MsgStatus::SUCCESS));

    ChatDao dao;
    ConversationModel conv;
    if (!dao.getConversationByConvId(rec.getSessionId(), conv))
    {
        conv.setConvId(rec.getSessionId());
        conv.setConvType(1);
        if (rec.getSendUserId() < rec.getReceiveId())
        {
            conv.setUser1Id(rec.getSendUserId());
            conv.setUser2Id(rec.getReceiveId());
        }
        else
        {
            conv.setUser1Id(rec.getReceiveId());
            conv.setUser2Id(rec.getSendUserId());
        }
        conv.setGroupId("");
        conv.setUser1UnreadCount(0);
        conv.setUser2UnreadCount(0);
    }

    conv.setLastMsg(rec.getMsgType() == ChatRecord::MsgType::IMAGE
        ? "[图片]" : rec.getMsgContent());
    conv.setLastMsgId(std::to_string(rec.getMsgId()));
    conv.setLastSenderId(rec.getSendUserId());
    if (rec.getReceiveId() == conv.getUser1Id())
        conv.setUser1UnreadCount(conv.getUser1UnreadCount() + 1);
    else if (rec.getReceiveId() == conv.getUser2Id())
        conv.setUser2UnreadCount(conv.getUser2UnreadCount() + 1);
    else
    {
        result["error"] = "conversation participants do not match";
        return result;
    }
    conv.setUpdateTime(rec.getSendTime());
    conv.setUser1isVaild(1);
    conv.setUser2isValid(1);

    if (dao.insertChatRecordAndUpdateConversation(rec, conv))
    {
        result["code"] = 100;
        result["sessionId"] = rec.getSessionId();
        result["sendTime"] = Json::UInt64(rec.getSendTime());
    }

    return result;
}

std::string ChatService::handleMessage(const Json::Value& jsonMsg)
{
    // 从 JSON 中提取需要转发的字段
    std::string senderName = jsonMsg["sendUserId"].asString();
    std::string content = jsonMsg["msgContent"].asString();
    Json::UInt64 sendTime = jsonMsg.isMember("sendTime")
        ? jsonMsg["sendTime"].asUInt64()
        : 0;
    Json::UInt64 msgId = jsonMsg.isMember("msgId")
        ? jsonMsg["msgId"].asUInt64()
        : 0;

    Json::Value forward;
    forward["type"] = "message";
    forward["msgContent"] = content;
    forward["sendUserId"] = senderName;
    forward["sendTime"] = sendTime;
    forward["msgId"] = msgId;
    forward["receiveId"] = jsonMsg["receiveId"];
    forward["receiveType"] = jsonMsg["receiveType"];
    forward["msgType"] = jsonMsg["msgType"];
    forward["msgStatus"] = 1;
    forward["sessionId"] = jsonMsg["sessionId"];
    forward["extendInfo"] = jsonMsg["extendInfo"];

    Json::StreamWriterBuilder wbuilder;
    std::string forwardStr = Json::writeString(wbuilder, forward);
    return forwardStr;

}
std::string ChatService::messageRead(const Json::Value& jsonMsg)
{
    //消息id
    uint64_t typeId = jsonMsg["msgId"].asUInt64();
	//sender此时进入聊天框则清楚sender的未读消息
	std::string sendId = jsonMsg["sender"].asString();
	std::string sessionId = jsonMsg["sessionId"].asString();
	ChatDao dao;
	//更新聊天记录表中消息状态为已读
    dao.updateMsgStatusByMsgId(typeId,3);
	//更新会话表中未读消息数为0
	dao.resetUnreadCountForUser(sessionId, sendId);
    //返回确认已读消息
    Json::Value forward;
    forward["msgId"] = typeId;
    forward["type"] = "read_ack";
    Json::StreamWriterBuilder wbuilder;
    std::string forwardStr = Json::writeString(wbuilder, forward);
	return forwardStr;
}

std::string ChatService::messageDelivered(const Json::Value& jsonMsg)
{
    uint64_t typeId = jsonMsg["msgId"].asUInt64();
	//表示发送成功已经收到但是未读
    ChatDao dao;
    dao.updateMsgStatusByMsgId(typeId, 1);
    Json::Value forward;
    forward["msgId"] = typeId;
    forward["type"] = "delivery_ack";
    forward["status"] = "sent";
    Json::StreamWriterBuilder wbuilder;
    std::string forwardStr = Json::writeString(wbuilder, forward);
    return forwardStr;
   
}

std::string ChatService::messageFailed(
    const Json::Value& jsonMsg,
    const std::string& reason)
{
    Json::Value response;
    response["type"] = "delivery_ack";
    response["msgId"] = jsonMsg["msgId"];
    response["status"] = "failed";
    response["reason"] = reason;
    Json::StreamWriterBuilder builder;
    return Json::writeString(builder, response);
}

Json::Value ChatService::getunReadMessage(const Json::Value& jsonMsg)
{
    Json::Value result;
    std::string userName = jsonMsg["userName"].asString();
    ChatDao dao;
    auto convs = dao.getUnreadMessage(userName);
    Json::Value arr(Json::arrayValue);
    for (const auto& c : convs) {
        Json::Value item;
        item["senderName"] = c.getSendUserId();
        item["receiverName"] = c.getReceiveId();
        item["timestamp"] = c.getSendTime();
        item["msgId"] = c.getMsgId();
        item["content"] = c.getMsgContent();
        item["messageType"] = (int)c.getMsgType() - 1;
        item["messageStatus"] = (int)c.getMsgStatus();
        item["conversationId"] = c.getSessionId();

        arr.append(item);
    }

    result["messageList"] = arr;
    return result;
}

Json::Value ChatService::getRecentChatRecords(const Json::Value& jsonMsg)
{
	Json::Value result;
	std::string sessionId = jsonMsg["conversationId"].asString();
	int limit = jsonMsg["limit"].asInt();
	ChatDao dao;
	auto convs = dao.getRecentChatRecordsBySessionId(sessionId, limit);
    Json::Value arr(Json::arrayValue);
    for (const auto& c : convs) {
        Json::Value item;
        item["senderName"] = c.getSendUserId();
        item["receiverName"] = c.getReceiveId();
        item["timestamp"] = c.getSendTime();
        item["msgId"] = c.getMsgId();
        item["content"] = c.getMsgContent();
        item["messageType"] = (int)c.getMsgType() - 1;
        item["messageStatus"] = (int)c.getMsgStatus();
        item["conversationId"] = c.getSessionId();

        arr.append(item);
    }

    result["messageList"] = arr;

           
    return result;
}

Json::Value ChatService::deletePrivateChatHistory(const Json::Value& jsonMsg)
{
    Json::Value result;
    result["code"] = 101;
    const std::string userName = jsonMsg["userName"].asString();
    const std::string peerUserName = jsonMsg["peerUserName"].asString();
    const std::string conversationId = jsonMsg["conversationId"].asString();
    if (userName.empty() || peerUserName.empty() || conversationId.empty())
    {
        result["code"] = 99;
        result["msg"] = "invalid request body";
        return result;
    }

    ChatDao dao;
    const int status = dao.deletePrivateChatHistory(
        conversationId, userName, peerUserName);
    if (status == 1)
    {
        result["code"] = 100;
    }
    else if (status == -1)
    {
        result["code"] = 103;
        result["msg"] = "conversation permission denied";
    }
    else
    {
        result["msg"] = "failed to delete chat history";
    }
    return result;
}

std::string ChatService::handleVideoCallRequest(const Json::Value& jsonMsg)
{
   
    //邀请方
    std::string senderId = jsonMsg["sender"].asString();
    Json::Value forward;
    forward["type"] = "videoCallInvite";
    forward["sender"] = senderId;
    forward["channelName"] = jsonMsg["channelName"].asString();;
    Json::StreamWriterBuilder wbuilder;
    std::string forwardStr = Json::writeString(wbuilder, forward);
    return forwardStr;
  
}

std::string ChatService::handleVideoCallAccept(const Json::Value& jsonMsg)
{
    //同意方
    std::string senderId = jsonMsg["sender"].asString();
    Json::Value forward;
    forward["type"] = "videoCallAccept";
    forward["sender"] = senderId;
    forward["channelName"] = jsonMsg["channelName"].asString();;
    Json::StreamWriterBuilder wbuilder;
    std::string forwardStr = Json::writeString(wbuilder, forward);
    return forwardStr;
}

std::string ChatService::handleVideoCallReject(const Json::Value& jsonMsg)
{
    //拒绝方
    std::string senderId = jsonMsg["sender"].asString();
    Json::Value forward;
    forward["type"] = "videoCallReject";
    forward["sender"] = senderId;
    forward["channelName"] = jsonMsg["channelName"].asString();;
    Json::StreamWriterBuilder wbuilder;
    std::string forwardStr = Json::writeString(wbuilder, forward);
    return forwardStr;
}

std::string ChatService::handleVideoCallEnd(const Json::Value& jsonMsg)
{
    //挂断方
    std::string senderId = jsonMsg["sender"].asString();
    Json::Value forward;
    forward["type"] = "videoCallHangup";
    forward["sender"] = senderId;
    forward["channelName"] = jsonMsg["channelName"].asString();;
    Json::StreamWriterBuilder wbuilder;
    std::string forwardStr = Json::writeString(wbuilder, forward);
    return forwardStr;
}
