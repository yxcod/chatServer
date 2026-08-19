#include "ChatRecoedModel.h"
#include <stdexcept>
#include"Logger.h"
// 1. 默认构造函数
ChatRecord::ChatRecord()
    : id(0), msgId(0), sessionId(""), sendUserId("0"), receiveType(ReceiveType::SINGLE_CHAT),
    receiveId("0"), msgType(MsgType::TEXT), msgStatus(MsgStatus::SENDING),
    sendTime(0), readTime(0) {
    extendInfo = "{}";
}

// 2. 带参数构造函数
ChatRecord::ChatRecord(uint64_t id, std::string sendUserId, ReceiveType receiveType,
    std::string receiveId, MsgType msgType, const std::string& msgContent,
    MsgStatus msgStatus, uint64_t sendTime, uint64_t readTime,
    const std::string& extendInfo) {
    this->id = id;
    this->msgId = 0; // 保持向后兼容，未提供参数时默认0
    this->sessionId = ""; // 默认空会话ID
    setSendUserId(sendUserId);
    this->receiveType = receiveType;
    setReceiveId(receiveId);
    this->msgType = msgType;
    this->msgContent = msgContent;
    this->msgStatus = msgStatus;
    this->sendTime = sendTime;
    this->readTime = readTime;
    this->extendInfo = extendInfo.empty() ? "{}" : extendInfo;
}

// 3. 拷贝构造函数
ChatRecord::ChatRecord(const ChatRecord& other) {
    this->id = other.id;
    this->msgId = other.msgId;
    this->sessionId = other.sessionId;
    this->sendUserId = other.sendUserId;
    this->receiveType = other.receiveType;
    this->receiveId = other.receiveId;
    this->msgType = other.msgType;
    this->msgContent = other.msgContent;
    this->msgStatus = other.msgStatus;
    this->sendTime = other.sendTime;
    this->readTime = other.readTime;
    this->extendInfo = other.extendInfo;
}

// 4. 赋值运算符重载
ChatRecord& ChatRecord::operator=(const ChatRecord& other) {
    if (this != &other) {
        this->id = other.id;
        this->msgId = other.msgId;
        this->sessionId = other.sessionId;
        this->sendUserId = other.sendUserId;
        this->receiveType = other.receiveType;
        this->receiveId = other.receiveId;
        this->msgType = other.msgType;
        this->msgContent = other.msgContent;
        this->msgStatus = other.msgStatus;
        this->sendTime = other.sendTime;
        this->readTime = other.readTime;
        this->extendInfo = other.extendInfo;
    }
    return *this;
}

// -------------------------- Getter 函数实现 --------------------------
uint64_t ChatRecord::getId() const {
    return id;
}

uint64_t ChatRecord::getMsgId() const {
    return msgId;
}

std::string ChatRecord::getSessionId() const {
    return sessionId;
}

std::string ChatRecord::getSendUserId() const {
    return sendUserId;
}

ChatRecord::ReceiveType ChatRecord::getReceiveType() const {
    return receiveType;
}

uint8_t ChatRecord::getReceiveTypeAsUInt8() const {
    return static_cast<uint8_t>(receiveType);
}

std::string ChatRecord::getReceiveId() const {
    return receiveId;
}

ChatRecord::MsgType ChatRecord::getMsgType() const {
    return msgType;
}

uint8_t ChatRecord::getMsgTypeAsUInt8() const {
    return static_cast<uint8_t>(msgType);
}

const std::string& ChatRecord::getMsgContent() const {
    return msgContent;
}

ChatRecord::MsgStatus ChatRecord::getMsgStatus() const {
    return msgStatus;
}

uint8_t ChatRecord::getMsgStatusAsUInt8() const {
    return static_cast<uint8_t>(msgStatus);
}

uint64_t ChatRecord::getSendTime() const {
    return sendTime;
}

uint64_t ChatRecord::getReadTime() const {
    return readTime;
}

const std::string& ChatRecord::getExtendInfo() const {
    return extendInfo;
}

// 可选：JSON扩展信息解析（需nlohmann/json库）
nlohmann::json ChatRecord::getExtendInfoAsJson() const {
    try {
        return nlohmann::json::parse(extendInfo);
    }
    catch (const nlohmann::json::parse_error& e) {
        return nlohmann::json::object();  // 解析失败返回空JSON
    }
}

// -------------------------- Setter 函数实现 --------------------------
void ChatRecord::setId(uint64_t id) {
    this->id = id;
}

void ChatRecord::setMsgId(uint64_t msgId) {
    this->msgId = msgId;
}

void ChatRecord::setSessionId(const std::string& sessionId) {
    this->sessionId = sessionId;
}

void ChatRecord::setSendUserId(std::string sendUserId) {
    this->sendUserId = sendUserId;  // 允许0（系统消息）
}

void ChatRecord::setReceiveType(ReceiveType receiveType) {
    this->receiveType = receiveType;
}

void ChatRecord::setReceiveType(uint8_t receiveType) {
    if (receiveType != static_cast<uint8_t>(ReceiveType::SINGLE_CHAT) &&
        receiveType != static_cast<uint8_t>(ReceiveType::GROUP_CHAT)) {
        throw std::invalid_argument("invalid receiveType: " + std::to_string(receiveType));
    }
    this->receiveType = static_cast<ReceiveType>(receiveType);
}

void ChatRecord::setReceiveId(std::string receiveId) {
    if (receiveId == "") {
        throw std::invalid_argument("receiveId cannot be 0");
    }
    this->receiveId = receiveId;
}

void ChatRecord::setMsgType(MsgType msgType) {
    this->msgType = msgType;
}

void ChatRecord::setMsgType(uint8_t msgType) {
    if (msgType < static_cast<uint8_t>(MsgType::TEXT) ||
        msgType > static_cast<uint8_t>(MsgType::CARD)) {
        throw std::invalid_argument("invalid msgType: " + std::to_string(msgType));
    }
    this->msgType = static_cast<MsgType>(msgType);
}

void ChatRecord::setMsgContent(const std::string& msgContent) {
    this->msgContent = msgContent;  // TEXT类型无长度限制，不校验
}

void ChatRecord::setMsgStatus(MsgStatus msgStatus) {
    this->msgStatus = msgStatus;
    // 已读状态自动设置当前时间为readTime，未读状态清除readTime
    if (msgStatus == MsgStatus::READ) {
        setReadTimeNow();
    }
    else if (msgStatus == MsgStatus::UNREAD) {
        clearReadTime();
    }
}

void ChatRecord::setMsgStatus(uint8_t msgStatus) {
    if (msgStatus < static_cast<uint8_t>(MsgStatus::SENDING) ||
        msgStatus > static_cast<uint8_t>(MsgStatus::UNREAD)) {
        throw std::invalid_argument("invalid msgStatus: " + std::to_string(msgStatus));
    }
    setMsgStatus(static_cast<MsgStatus>(msgStatus));
}

void ChatRecord::setSendTime(uint64_t sendTime) {
    this->sendTime = sendTime;
}

void ChatRecord::setSendTimeNow() {
    this->sendTime = Logger::GetInstance().getcurrentTime();
}

void ChatRecord::setReadTime(uint64_t readTime) {
    this->readTime = readTime;
    this->msgStatus = MsgStatus::READ;
}

void ChatRecord::setReadTimeNow() {
    this->readTime = Logger::GetInstance().getcurrentTime();
}

void ChatRecord::clearReadTime() {
    this->readTime = 0;
    this->msgStatus = MsgStatus::UNREAD;
}

void ChatRecord::setExtendInfo(const std::string& extendInfo) {
    this->extendInfo = extendInfo.empty() ? "{}" : extendInfo;
}

// 可选：直接传入JSON对象设置扩展信息
void ChatRecord::setExtendInfo(const nlohmann::json& json) {
    this->extendInfo = json.dump();
}

// -------------------------- 辅助函数实现 --------------------------
std::string ChatRecord::getReceiveTypeDesc() const {
    switch (receiveType) {
    case ReceiveType::SINGLE_CHAT: return "单聊";
    case ReceiveType::GROUP_CHAT: return "群聊";
    default: return "未知类型";
    }
}

std::string ChatRecord::getMsgTypeDesc() const {
    switch (msgType) {
    case MsgType::TEXT: return "文本";
    case MsgType::IMAGE: return "图片";
    case MsgType::VOICE: return "语音";
    case MsgType::VIDEO: return "视频";
    case MsgType::FILE: return "文件";
    case MsgType::EMOJI: return "表情";
    case MsgType::LOCATION: return "位置";
    case MsgType::CARD: return "名片";
    default: return "未知消息";
    }
}

std::string ChatRecord::getMsgStatusDesc() const {
    switch (msgStatus) {
    case MsgStatus::SENDING: return "发送中";
    case MsgStatus::SUCCESS: return "发送成功";
    case MsgStatus::FAILED: return "发送失败";
    case MsgStatus::READ: return "已读";
    case MsgStatus::UNREAD: return "未读";
    default: return "未知状态";
    }
}

std::string ChatRecord::toString() const {
    // 格式化时间戳为可读字符串
    auto formatTs = [](uint64_t ts) -> std::string {
        if (ts == 0) return "未读";
        time_t t = static_cast<time_t>(ts);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    };

    std::string sendTimeStr = formatTs(sendTime);
    std::string readTimeStr = (readTime == 0) ? "未读" : formatTs(readTime);

    std::stringstream result;
    result << "ChatRecord{"
        << "id=" << id
        << ", msgId=" << msgId
        << ", sessionId=" << sessionId
        << ", sendUserId=" << sendUserId
        << ", receiveType=" << getReceiveTypeDesc() << "(" << static_cast<uint8_t>(receiveType) << ")"
        << ", receiveId=" << receiveId
        << ", msgType=" << getMsgTypeDesc() << "(" << static_cast<uint8_t>(msgType) << ")"
        << ", msgContent='" << msgContent << '\''
        << ", msgStatus=" << getMsgStatusDesc() << "(" << static_cast<uint8_t>(msgStatus) << ")"
        << ", sendTime='" << sendTimeStr << '\''
        << ", readTime='" << readTimeStr << '\''
        << ", extendInfo='" << extendInfo << '\''
        << '}';
    return result.str();
}