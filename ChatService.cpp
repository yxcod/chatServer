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

    ChatDao dao;
    //图片
    if (rec.getMsgType() == ChatRecord::MsgType::IMAGE)
    {
		rec.setMsgContent("[图片]");

    }
    int affected = dao.insertChatRecord(rec);
    if (affected > 0) {
        // 插入聊天记录成功，同时更新会话表 conversations
        ConversationModel conv;
        // sessionId 即 convId
        conv.setConvId(rec.getSessionId());
        // 单聊会话，convType 默认为 1
        conv.setConvType(1);
        // 会话双方：user1Id / user2Id
        conv.setUser1Id(rec.getSendUserId());
        conv.setUser2Id(rec.getReceiveId());
        // 单聊 groupId 为空
        conv.setGroupId("");
        // 更新最后一条消息相关字段
        conv.setLastMsg(rec.getMsgContent());
        conv.setLastMsgId(std::to_string(rec.getId()));
        conv.setLastSenderId(rec.getSendUserId());

        // 从数据库中取出当前会话的未读计数
        int currentUser1Unread = 0;
        int currentUser2Unread = 0;
        auto existingConvs = dao.getUserAllConversation(rec.getSendUserId());
        for (const auto& c : existingConvs) {
            if (c.getConvId() == conv.getConvId()) {
                currentUser1Unread = c.getUser1UnreadCount();
                currentUser2Unread = c.getUser2UnreadCount();
                break;
            }
        }
        conv.setUser1UnreadCount(currentUser1Unread);
        conv.setUser2UnreadCount(currentUser2Unread);

        // 未读计数：如果发送者是 user1，则给 user2 未读 +1；否则给 user1 未读 +1
        if (rec.getSendUserId() == conv.getUser1Id()) {
            conv.setUser2UnreadCount(conv.getUser2UnreadCount() + 1);
        }
        else {
            conv.setUser1UnreadCount(conv.getUser1UnreadCount() + 1);
        }
        // 更新时间使用消息发送时间
        conv.setUpdateTime(rec.getSendTime());
        conv.setUser1isVaild(1);
        conv.setUser2isValid(1);

        dao.updateConversation(conv);

        result["code"] = 100;
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

    // 构造转发的精简消息：只包含内容、发送者、时间和 msgId
    Json::Value forward;
    forward["type"] = "message";
    forward["msgContent"] = content;
    forward["sendUserId"] = senderName;
    forward["sendTime"] = sendTime;
    forward["msgId"] = msgId;

    Json::StreamWriterBuilder wbuilder;
    std::string forwardStr = Json::writeString(wbuilder, forward);
    return forwardStr;

}
std::string ChatService::messageRead(const Json::Value& jsonMsg)
{
    //消息id
    UINT64 typeId = jsonMsg["msgId"].asUInt64();
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
    UINT64 typeId = jsonMsg["msgId"].asUInt64();
	//表示发送成功已经收到但是未读
    ChatDao dao;
    dao.updateMsgStatusByMsgId(typeId, 1);
    Json::Value forward;
    forward["msgId"] = typeId;
    forward["type"] = "delivery_ack";
    Json::StreamWriterBuilder wbuilder;
    std::string forwardStr = Json::writeString(wbuilder, forward);
    return forwardStr;
   
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
        item["timnestamp"] = c.getSendTime();
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
