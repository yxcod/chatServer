#pragma once
#include"ConversationModel.h"
#include "ChatRecoedModel.h"
#include <string>
#include <ctime>  
#include <sstream>
#include <iomanip>
#include <vector>
#include "Logger.h"
class ChatDao
{
public:
	std::vector<ConversationModel> getUserAllConversation(const std::string& userId) const;
	
	//删除会话记录，返回受影响行数
	int deleteConversationByConvId(const std::string& convId) const;
	// insert or update conversation record: returns affected rows (1 on success, 0 on failure)
	int updateConversation(const ConversationModel &conversation) const;
	bool getConversationByConvId(const std::string& convId,
		ConversationModel& conversation) const;
	bool insertChatRecordAndUpdateConversation(
		const ChatRecord& record,
		const ConversationModel& conversation) const;

	// -- Chat record (chatrecord) related operations (新增，不修改已有代码)
	// 查询接收方 receiveId 的最近聊天记录（按 sendTime 降序），limit 默认为 100
	std::vector<ChatRecord> getChatRecordsByReceiveId(const std::string& receiveId, size_t limit = 100) const;

	//获取用户的所有未读记录
	std::vector<ChatRecord> getUnreadMessage(const std::string& userName);

	// 插入一条聊天记录，返回受影响行数（或 0 表示失败）
	int insertChatRecord(const ChatRecord &record) const;

	// 根据消息 id 删除聊天记录，返回受影响行数
	int deleteChatRecordById(uint64_t id) const;

	// 删除指定会话（sessionId）下的所有聊天记录，返回受影响行数
	int deleteChatRecordsBetweenUsers(const std::string& sessionId) const;

	// 原子删除私聊会话及其全部消息。仅当请求者和对方确为会话参与者时执行。
	// 1=成功（包含重复删除），-1=无权限或会话参与者不匹配，0=数据库错误。
	int deletePrivateChatHistory(const std::string& sessionId,
		const std::string& requesterId,
		const std::string& peerId) const;

	// 根据会话ID sessionId 查询 chatrecord 表中的所有聊天记录
	std::vector<ChatRecord> getChatRecordsBySessionId(const std::string& sessionId) const;

	// 根据业务消息ID msgId 更新 chatrecord 表中的 msgStatus
	int updateMsgStatusByMsgId(uint64_t msgId, uint8_t msgStatus) const;

	// 将指定会话 convId 中对应用户的未读数清零
	// 如果 userName == user1Id，则将 user1UnreadCount 置为 0
	// 如果 userName == user2Id，则将 user2UnreadCount 置为 0
	int resetUnreadCountForUser(const std::string& convId, const std::string& userName) const;
	// 获取指定会话 sessionId 下的最近 limit 条聊天记录，按发送时间降序排列
	std::vector<ChatRecord> getRecentChatRecordsBySessionId(const std::string& sessionId, int limit) const;
};
