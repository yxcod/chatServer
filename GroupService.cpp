#include "GroupService.h"
#include"Logger.h"
#include "UserInfoDao.h"
Json::Value GroupService::getAllGroups(const Json::Value& groupInfo)
{
	// 获取用户所有群组信息
	Json::Value response;
	GroupChatDao groupDao;
	std::string userId = groupInfo["userName"].asString();
	auto groups = groupDao.getGroupsByUserId(userId);
	response["code"] = 101;
	if (groups.size() == 0)
	{
		return response;
	}
	Json::Value groupArray(Json::arrayValue);
	for (const auto& group : groups) {
		Json::Value groupJson;
		groupJson["groupId"] = Json::UInt64(group.getGroupId());
		groupJson["groupName"] = group.getGroupName();
		groupJson["groupAvatar"] = group.getGroupAvatar();
		groupJson["creatorId"] = group.getCreatorId();
		groupJson["description"] = group.getDescription();
		groupJson["maxMembers"] = group.getMaxMembers();
		groupJson["isActive"] = group.getIsActive();
		groupJson["createdAt"] = group.getCreatedAt();
		groupJson["updatedAt"] = group.getUpdatedAt();
		groupArray.append(groupJson);
	}
	response["code"] = 100;
	response["groups"] = groupArray;
	return response;
       
    
}

Json::Value GroupService::getGroupMembers(const Json::Value& groupInfo)
{
	Json::Value response;
	response["code"] = 101; // 默认失败
	int groupId = groupInfo["groupId"].asInt();
	if (groupId <= 0)
	{
		response["msg"] = "invalid groupId";
		return response;
	}
	uint64_t groupId64 = static_cast<uint64_t>(groupId);

	GroupMemberDao groupMemberDao;
	auto members = groupMemberDao.getMembersByGroup(groupId64);

	if (members.empty())
	{
		// 没有成员也视为成功，只是列表为空
		response["code"] = 100;
		response["groupId"] = Json::UInt64(groupId64);
		response["members"] = Json::Value(Json::arrayValue);
		return response;
	}

	Json::Value arr(Json::arrayValue);
	UserInfoDao userInfodao;
	for (const auto& m : members)
	{
		Json::Value item;
		UserInfo userInfo = userInfodao.getUserinfo(m.getUserId());
		item["id"] = Json::UInt64(m.getId());
		item["groupId"] = Json::UInt64(m.getGroupId());
		item["userId"] = m.getUserId();
		item["role"] = static_cast<int>(m.getRole());              // 0-普通 1-管理员 2-群主
		item["joinTime"] = Json::UInt64(m.getJoinTime());
		item["quitTime"] = Json::UInt64(m.getQuitTime());
		item["isQuit"] = static_cast<int>(m.getIsQuit());          // 0-未退出 1-已退出
		item["groupNickName"] = m.getGroupNickName();
		item["avatar"] = userInfo.getAvatar();
		arr.append(item);
	}

	response["code"] = 100;
	response["groupId"] = Json::UInt64(groupId64);
	response["members"] = arr;
	return response;
}

Json::Value GroupService::getGroupChatRecord(const Json::Value& groupInfo)
{
	Json::Value response;
	response["code"] = 101;
	int groupId = groupInfo["groupId"].asInt();
	uint64_t groupId64 = static_cast<uint64_t>(groupId);

	// 2. 查询该群所有消息
	GroupMessageDao msgDao;
	// 获取最近 limit 条记录
	std::size_t limit = groupInfo["limit"].asInt();
	auto messages = msgDao.getRecentMessages(groupId64, limit);

	// 3. 为每条消息查已读信息
	GroupMsgReadDao readDao;

	Json::Value msgArray(Json::arrayValue);
	for (const auto& m : messages)
	{
		Json::Value msgJson;
		msgJson["msgId"] = Json::UInt64(m.getMsgId());
		msgJson["groupId"] = Json::UInt64(m.getGroupId());
		msgJson["senderId"] = m.getSenderId();
		msgJson["msgType"] = static_cast<int>(m.getMsgType());
		msgJson["msgContent"] = m.getMsgContent();
		msgJson["fileSize"] = Json::UInt64(m.getFileSize());
		msgJson["sendTime"] = Json::UInt64(m.getSendTime());
		msgJson["isDeleted"] = static_cast<int>(m.getIsDeleted());
		msgJson["isRead"] = static_cast<int>(m.getIsRead());

		// 3.1 查询该消息的已读用户列表
		auto readers = readDao.getReadersByMsg(m.getMsgId());
		Json::Value readArray(Json::arrayValue);
		for (const auto& r : readers)
		{
			Json::Value rJson;
			rJson["userId"] = r.getUserId();
			rJson["readTime"] = Json::UInt64(r.getReadTime());
			readArray.append(rJson);
		}
		msgJson["readers"] = readArray;

		msgArray.append(msgJson);
	}

	response["code"] = 100;
	response["groupId"] = Json::UInt64(groupId64);
	response["messages"] = msgArray;
	return response;
}

Json::Value GroupService::getGroupConversations(const Json::Value& userInfo)
{
	std::string userId = userInfo["userName"].asString();
	Json::Value response;
	response["code"] = 101;
	GroupConversationDao groupConversationDao;
	auto conversations = groupConversationDao.getConversationsByUser(userId);
	if (conversations.size() == 0)
	{
		return response;

	}
	Json::Value msgArray(Json::arrayValue);
	for (const auto& convs : conversations)
	{
		GroupMessageDao msgDao;
		Json::Value msgJson;
		msgJson["groupId"] = Json::UInt64(convs.getGroupId());
		msgJson["updateTime"] = Json::UInt64(convs.getUpdateTime());
		msgJson["lastSenderId"] = convs.getLastSenderId();
		msgJson["lastMsg"] = convs.getLastMsg();
		//图片信息
		if (convs.getMsgType() == 1)
		{
			msgJson["lastMsg"] ="[图片]";
		}
		msgJson["unreadCount"] = msgDao.getUnreadCountByUserAndGroup(userId, convs.getGroupId());
		msgArray.append(msgJson);
	}
	response["conversations"] = msgArray;
	response["code"] = 100;
	return response;
}

Json::Value GroupService::createGroup(const Json::Value& groupInfo)
{
	Json::Value response;
	GroupChatDao groupDao;
	std::string userId = groupInfo["createUserName"].asString();
	int groupId = groupInfo["groupId"].asInt();
	std::string groupName = groupInfo["groupName"].asString();
	GroupChatModel group;
	group.setGroupId(static_cast<uint64_t>(groupId));
	group.setGroupName(groupName);
	group.setCreatedAt(Logger::GetInstance().getcurrentTime());
	group.setUpdatedAt(Logger::GetInstance().getcurrentTime());
	group.setDescription("This is a new group");
	group.setIsActive(1);
	group.setGroupAvatar("head.jpg");
	group.setMaxMembers(200);
	group.setCreatorId(userId);
	response["code"] = 101;
	//群ID重复
	if (groupDao.groupExists(groupId))
	{
		response["code"] = 102;
		return response;

	}
	GroupMemberDao groupMemberDao;
	UserInfoDao userInfoDao;
	UserInfo userInfo = userInfoDao.getUserinfo(userId);
	GroupMemberModel member;
	member.setGroupId(static_cast<uint64_t>(groupId));
	member.setUserId(userId);
	member.setRole(2); //群主
	member.setJoinTime(Logger::GetInstance().getcurrentTime());
	member.setIsQuit(0);
	member.setQuitTime(0);
	member.setGroupNickName(userInfo.getNickName());
	//创建成功
	if (groupDao.createGroup(group) > 0 && groupMemberDao.addMember(member) > 0)
	{
		response["code"] = 100;
		//创建群数据存储目录
		Logger::GetInstance().createDataDirectories(std::to_string(groupId));
	}
	
	return response;
}

Json::Value GroupService::addGroupMember(const Json::Value& memberInfo)
{
	Json::Value response;
	response["code"] = 101; // 默认失败

	// 1. 基本参数校验：必须有 groupId 和 users（数组）
	if (!memberInfo["userNames"].isArray())
	{
		response["msg"] = "missing groupId or users(array)";
		return response;
	}

	int groupId = memberInfo["groupId"].asInt();
	if (groupId <= 0)
	{
		response["msg"] = "invalid groupId";
		return response;
	}
	uint64_t groupId64 = static_cast<uint64_t>(groupId);

	const Json::Value& users = memberInfo["userNames"];
	if (users.empty())
	{
		response["msg"] = "users is empty";
		return response;
	}

	GroupChatDao groupChatDao;
	GroupMemberDao groupMemberDao;
	UserInfoDao userInfoDao;

	// 2. 检查群是否存在
	if (!groupChatDao.groupExists(groupId64))
	{
		response["code"] = 102;
		response["msg"] = "group not exists";
		return response;
	}

	// 3. 逐个处理用户
	Json::Value resultArray(Json::arrayValue);
	bool anySuccess = false;

	for (const auto& userNode : users)
	{
		Json::Value itemResult;
		itemResult["code"] = 101;

		if (!userNode.isString())
		{
			itemResult["msg"] = "user item not string";
			resultArray.append(itemResult);
			continue;
		}

		std::string userId = userNode.asString();
		itemResult["userNames"] = userId;

		if (userId.empty())
		{
			itemResult["msg"] = "empty userId";
			resultArray.append(itemResult);
			continue;
		}

		// 已在群中（未退出）
		if (groupMemberDao.isUserInGroup(groupId64, userId))
		{
			itemResult["code"] = 103;
			itemResult["msg"] = "user already in group";
			resultArray.append(itemResult);
			continue;
		}

		// 获取用户信息
		UserInfo userInfo = userInfoDao.getUserinfo(userId);

		GroupMemberModel member;
		member.setGroupId(groupId64);
		member.setUserId(userId);
		member.setRole(0); // 默认普通成员
		member.setJoinTime(Logger::GetInstance().getcurrentTime());
		member.setQuitTime(0);
		member.setIsQuit(0);
		member.setGroupNickName(userInfo.getNickName());

		if (groupMemberDao.addMember(member))
		{
			anySuccess = true;
			itemResult["code"] = 100;
			itemResult["groupNickName"] = member.getGroupNickName();
		}
		else
		{
			itemResult["msg"] = "db error when add member";
		}

		resultArray.append(itemResult);
	}

	// 顶层 code：只要有一个成功就置为 100
	response["code"] = anySuccess ? 100 : 101;
	response["groupId"] = Json::UInt64(groupId64);
	//返回成功添加到群内的用户列表
	response["results"] = resultArray;
	return response;
}

Json::Value GroupService::minuGroupMember(const Json::Value& memberInfo)
{
	Json::Value response;
	response["code"] = 101; // 默认失败

	// 1. 基本参数校验：必须有 groupId 和 users（数组）
	if (!memberInfo["userNames"].isArray())
	{
		response["msg"] = "missing groupId or users(array)";
		return response;
	}

	int groupId = memberInfo["groupId"].asInt();
	if (groupId <= 0)
	{
		response["msg"] = "invalid groupId";
		return response;
	}
	uint64_t groupId64 = static_cast<uint64_t>(groupId);

	const Json::Value& users = memberInfo["userNames"];
	if (users.empty())
	{
		response["msg"] = "users is empty";
		return response;
	}

	GroupChatDao groupChatDao;
	GroupMemberDao groupMemberDao;
	UserInfoDao userInfoDao;

	// 2. 检查群是否存在
	if (!groupChatDao.groupExists(groupId64))
	{
		response["code"] = 102;
		response["msg"] = "group not exists";
		return response;
	}

	// 2.1 查询群信息，获取创建者 ID
	GroupChatModel groupModel = groupChatDao.getGroupById(groupId64);
	std::string creatorId = groupModel.getCreatorId();

	// 标记是否需要解散群（有人被移除且是群创建者）
	bool needDissolveGroup = false;

	// 3. 逐个处理用户
	Json::Value resultArray(Json::arrayValue);
	bool anySuccess = false;

	for (const auto& userNode : users)
	{
		Json::Value itemResult;
		itemResult["code"] = 101;

		if (!userNode.isString())
		{
			itemResult["msg"] = "user item not string";
			resultArray.append(itemResult);
			continue;
		}

		std::string userId = userNode.asString();
		itemResult["userNames"] = userId;

		if (userId.empty())
		{
			itemResult["msg"] = "empty userId";
			resultArray.append(itemResult);
			continue;
		}

		// 先标记用户退群
		if (groupMemberDao.markQuit(groupId64, userId, Logger::GetInstance().getcurrentTime()))
		{
			anySuccess = true;
			itemResult["code"] = 100;

			// 如果这个被移除的用户是群创建者，则后面触发解散群逻辑
			if (!creatorId.empty() && userId == creatorId)
			{
				needDissolveGroup = true;
			}
		}
		else
		{
			itemResult["msg"] = "db error when mark quit";
		}

		resultArray.append(itemResult);
	}

	// 4. 如有需要，执行解散群逻辑：删除群相关所有数据
	bool dissolveOk = true;
	if (needDissolveGroup)
	{
		GroupMessageDao msgDao;
		GroupConversationDao convDao;

		bool okMsg = msgDao.deleteMessagesByGroupId(groupId64);
		bool okMembers = groupMemberDao.deleteByGroupId(groupId64);
		bool okConv = convDao.deleteByGroupId(groupId64);
		bool okGroup = groupChatDao.deleteGroupById(groupId64);

		dissolveOk = okMsg && okMembers && okConv && okGroup;

		response["groupDissolved"] = dissolveOk ? 1 : 0;
	}

	// 顶层 code：只要有一个成功就置为 100
	response["code"] = anySuccess ? 100 : 101;
	response["groupId"] = Json::UInt64(groupId64);
	// 返回每个用户的处理结果
	response["results"] = resultArray;
	return response;
}

Json::Value GroupService::updateGroupMemberInfo(const Json::Value& memberInfo)
{
	Json::Value response;
	response["code"] = 101; // 默认失败
	GroupMemberDao groupMemberDao;
	uint64_t groupId = memberInfo["groupId"].asUInt64();
	std::string userId = memberInfo["userName"].asString();
	//更新成员昵称
	if (memberInfo.isMember("nickName"))
	{
		std::string nickName = memberInfo["nickName"].asString();
		if (groupMemberDao.updateGroupNickName(groupId, userId, nickName))
		{
			response["code"] = 100;
			return response;
		}

	}
	//更新成员角色
	if (memberInfo.isMember("role"))
	{
		int roleId = memberInfo["role"].asInt();
		if (groupMemberDao.updateGroupRole(groupId, userId, roleId))
		{
			response["code"] = 100;
			return response;
		}

	}
	return response;
}

Json::Value GroupService::updateGroupInfo(const Json::Value& groupInfo)
{
	Json::Value response;
	response["code"] = 101; // 默认失败

	uint64_t groupId = groupInfo["groupId"].asUInt64();
	if (groupId == 0)
	{
		response["msg"] = "invalid groupId";
		return response;
	}

	GroupChatDao groupDao;

	// 2. 检查群是否存在
	if (!groupDao.groupExists(groupId))
	{
		response["code"] = 102;
		response["msg"] = "group not exists";
		return response;
	}

	// 3. 构造“部分更新”的 GroupChatModel
	GroupChatModel patch;

	// 字段约定：
	// - 非空字符串才更新：groupName / groupAvatar / description
	// - maxMembers != 0 才更新
	// - isActive != 255 才更新（调用方如果不想改状态就不要传，或传 255）

	if (groupInfo.isMember("groupName") && groupInfo["groupName"].isString())
	{
		std::string name = groupInfo["groupName"].asString();
		if (!name.empty())
		{
			patch.setGroupName(name);
		}
	}

	if (groupInfo.isMember("groupAvatar") && groupInfo["groupAvatar"].isString())
	{
		std::string avatar = groupInfo["groupAvatar"].asString();
		if (!avatar.empty())
		{
			patch.setGroupAvatar(avatar);
		}
	}

	if (groupInfo.isMember("description") && groupInfo["description"].isString())
	{
		std::string desc = groupInfo["description"].asString();
		if (!desc.empty())
		{
			patch.setDescription(desc);
		}
	}

	if (groupInfo.isMember("maxMembers") && groupInfo["maxMembers"].isUInt())
	{
		uint32_t maxMembers = groupInfo["maxMembers"].asUInt();
		if (maxMembers != 0)
		{
			patch.setMaxMembers(maxMembers);
		}
	}

	if (groupInfo.isMember("isActive") && groupInfo["isActive"].isInt())
	{
		int active = groupInfo["isActive"].asInt();
		// 约定：-1 表示“不修改”，其它 0/1 有效
		if (active == 0 || active == 1)
		{
			patch.setIsActive(static_cast<uint8_t>(active));
		}
		else
		{
			// 不修改 isActive 的话就设为 255，让 Dao 层跳过
			patch.setIsActive(static_cast<uint8_t>(255));
		}
	}
	else
	{
		// 未提供 isActive，显式标记为“不更新”
		patch.setIsActive(static_cast<uint8_t>(255));
	}

	// 4. 调用 Dao 执行更新（只更新 patch 中“有值”的字段）
	if (groupDao.updateGroupInfo(groupId, patch))
	{
		response["code"] = 100;
		response["groupId"] = Json::UInt64(groupId);
	}
	else
	{
		response["msg"] = "db error when update group info";
	}

	return response;
}

std::vector<std::string> GroupService::getUserIds(const int& groupId)
{
	GroupMemberDao groupMemberDao;
	auto members = groupMemberDao.getMembersByGroup(static_cast<uint64_t>(groupId));
	std::vector<std::string> userIds;
	if (members.size() > 0)
	{
		
		for (const auto& member : members)
		{
			userIds.push_back(member.getUserId());
		}
		return userIds;
	}
	return userIds;
}

std::string GroupService::handleGroupMessage(const Json::Value& jsonMsg)
{

	Json::Value response = jsonMsg;
	std::string sender = jsonMsg["sendUserId"].asString();
	std::string content = jsonMsg["msgContent"].asString();
	int groupId = jsonMsg["receiveId"].asInt();
	int msgType = jsonMsg["msgType"].asInt();
	uint64_t msgId = jsonMsg["msgId"].asUInt64();
	response["code"] = 101;
	GroupMessageDao groupMessageDao;
	GroupMessageModel msg;
	msg.setMsgId(msgId);
	msg.setFileSize(0);
	msg.setGroupId(static_cast<uint64_t>(groupId));
	msg.setIsDeleted(0);
	msg.setIsRead(0);
	msg.setMsgContent(content);
	msg.setMsgType(static_cast<uint8_t>(msgType));
	msg.setSenderId(sender);
	msg.setSendTime(Logger::GetInstance().getcurrentTime());
	
	GroupMsgReadDao groupMsgReadDao;
	std::vector<GroupMsgReadModel> readModels;
	std::vector<std::string> userIds = getUserIds(groupId);
	if (userIds.size() > 0)
	{
		for (const auto& member : userIds)
		{
			// 排除发送者自己
			if(member==sender)
				continue;
			GroupMsgReadModel readModel;
			readModel.setMsgId(msgId);
			readModel.setReadTime(0);
			readModel.setUserId(member);
			readModels.push_back(readModel);
		}	
		
	}
	//更新群的会话表
	GroupConversationDao groupConvDao;
	GroupConversationModel groupConv;
	groupConv.setGroupId(static_cast<uint64_t>(groupId));
	groupConv.setLastMsg(content);
	groupConv.setLastSenderId(sender);
	groupConv.setUpdateTime(Logger::GetInstance().getcurrentTime());
	groupConv.setValidList(Logger::GetInstance().joinWithUnderscore(userIds));

	if (groupMsgReadDao.insertBatch(readModels)>0 && groupMessageDao.insertMessage(msg) > 0 && groupConvDao.insert(groupConv) > 0)
	{
		response["code"] = 100;

	}
	Json::StreamWriterBuilder wbuilder;
	std::string forwardStr = Json::writeString(wbuilder, response);
	return forwardStr;
	
}

std::string GroupService::groupMessageRead(const Json::Value& jsonMsg)
{
	Json::Value response = jsonMsg;
	response["code"] = 101;
	GroupMsgReadDao groupMsgReadDao;
	GroupMsgReadModel groupMsgReadModel;
	//已读消息的用户ID
	std::string sender = jsonMsg["sender"].asString();
	uint64_t msgId = jsonMsg["msgId"].asUInt64();
	groupMsgReadModel.setUserId(sender);
	groupMsgReadModel.setMsgId(msgId);
	if (groupMsgReadDao.markRead(groupMsgReadModel) > 0)
	{
		response["code"] = 100;
	}
	Json::StreamWriterBuilder wbuilder;
	std::string forwardStr = Json::writeString(wbuilder, response);
	return forwardStr;
}
