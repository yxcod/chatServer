#ifndef CHAT_RECORD_H
#define CHAT_RECORD_H

#include <string>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>  // 可选：用于JSON格式扩展信息（需自行安装nlohmann/json库）

/**
 * 聊天记录模型（对应 MySQL chat_record 表）
 * 字段映射：
 * id           → BIGINT UNSIGNED → uint64_t（消息唯一ID）
 * msgId        → BIGINT UNSIGNED → uint64_t（业务/第三方消息ID，可用于去重或跨系统关联）
 * sendUserId   → BIGINT UNSIGNED → uint64_t（发送方用户ID，0=系统）
 * receiveType  → TINYINT         → uint8_t（1=单聊，2=群聊）
 * receiveId    → BIGINT UNSIGNED → uint64_t（接收方ID：用户ID/群ID）
 * msgType      → TINYINT         → uint8_t（1=文本，2=图片，3=语音...）
 * msgContent   → TEXT            → std::string（消息内容/媒体URL/JSON）
 * msgStatus    → TINYINT         → uint8_t（0=发送中，1=成功，2=失败，3=已读，4=未读）
 * sendTime     → BIGINT UNSIGNED → uint64_t（Unix 时间戳）
 * readTime     → BIGINT UNSIGNED → uint64_t（Unix 时间戳，可为0表示未读）
 * extendInfo   → VARCHAR(512)    → std::string（JSON格式扩展信息）
 * sessionId    → VARCHAR(64)     → std::string（会话ID，字符串类型，用于标识会话/convId等）
 */
class ChatRecord {
public:
    // 接收类型枚举（单聊/群聊）
    enum class ReceiveType : uint8_t {
        SINGLE_CHAT = 1,  // 单聊（接收方是用户）
        GROUP_CHAT = 2    // 群聊（接收方是群）
    };

    // 消息类型枚举（支持常见消息类型，可扩展）
    enum class MsgType : uint8_t {
        TEXT = 1,     // 文本
        IMAGE = 2,    // 图片
        VOICE = 3,    // 语音
        VIDEO = 4,    // 视频
        FILE = 5,     // 文件
        EMOJI = 6,    // 表情
        LOCATION = 7, // 位置
        CARD = 8      // 名片
    };

    // 消息状态枚举（消息生命周期）
    enum class MsgStatus : uint8_t {
        SENDING = 0,  // 发送中
        SUCCESS = 1,  // 发送成功
        FAILED = 2,   // 发送失败
        READ = 3,     // 已读
        UNREAD = 4    // 未读
    };

    // 1. 默认构造函数
    ChatRecord();

    // 2. 带参数构造函数（快速初始化所有字段）
    ChatRecord(uint64_t id, std::string sendUserId, ReceiveType receiveType,
        std::string receiveId, MsgType msgType, const std::string& msgContent,
        MsgStatus msgStatus, uint64_t sendTime, uint64_t readTime,
        const std::string& extendInfo);

    // 3. 拷贝构造函数
    ChatRecord(const ChatRecord& other);

    // 4. 赋值运算符重载
    ChatRecord& operator=(const ChatRecord& other);

    // 5. 析构函数
    ~ChatRecord() = default;

    // -------------------------- Getter 函数 --------------------------
    uint64_t getId() const;
    uint64_t getMsgId() const; // 新增: 消息在第三方或业务层的消息ID
    std::string getSessionId() const; // 新增: 会话ID（字符串）
    std::string getSendUserId() const;
    ReceiveType getReceiveType() const;
    uint8_t getReceiveTypeAsUInt8() const;  // 辅助：返回uint8_t（数据库存储用）
    std::string getReceiveId() const;
    MsgType getMsgType() const;
    uint8_t getMsgTypeAsUInt8() const;      // 辅助：返回uint8_t（数据库存储用）
    const std::string& getMsgContent() const;
    MsgStatus getMsgStatus() const;
    uint8_t getMsgStatusAsUInt8() const;    // 辅助：返回uint8_t（数据库存储用）
    uint64_t getSendTime() const;           // 返回 Unix 时间戳
    uint64_t getReadTime() const;           // 返回 Unix 时间戳，0 表示未读
    const std::string& getExtendInfo() const;

    // 可选：JSON扩展信息解析（需nlohmann/json库）
    nlohmann::json getExtendInfoAsJson() const;

    // -------------------------- Setter 函数 --------------------------
    void setId(uint64_t id);
    void setMsgId(uint64_t msgId); // 新增 setter
    void setSessionId(const std::string& sessionId); // 新增 setter
    void setSendUserId(std::string sendUserId);  // 0=系统消息，其他=用户ID
    void setReceiveType(ReceiveType receiveType);
    void setReceiveType(uint8_t receiveType); // 重载：支持uint8_t输入
    void setReceiveId(std::string receiveId);    // 校验：接收ID不能为0
    void setMsgType(MsgType msgType);
    void setMsgType(uint8_t msgType);         // 重载：支持uint8_t输入
    void setMsgContent(const std::string& msgContent);  // 文本消息无长度限制（TEXT类型）
    void setMsgStatus(MsgStatus msgStatus);
    void setMsgStatus(uint8_t msgStatus);     // 重载：支持uint8_t输入
    void setSendTime(uint64_t sendTime);
    void setSendTimeNow();                    // 便捷：设置发送时间为当前时间戳
    void setReadTime(uint64_t readTime);     // 标记为已读（设置读取时间戳）
    void setReadTimeNow();                    // 便捷：设置读取时间为当前时间戳
    void clearReadTime();                     // 清除读取时间（恢复未读状态）
    void setExtendInfo(const std::string& extendInfo);  // 设置扩展信息（JSON字符串）
    void setExtendInfo(const nlohmann::json& json);      // 可选：直接传入JSON对象

    // -------------------------- 辅助函数 --------------------------
    std::string toString() const;              // 对象转字符串（调试用）
    std::string getReceiveTypeDesc() const;    // 获取接收类型描述（如"单聊"）
    std::string getMsgTypeDesc() const;        // 获取消息类型描述（如"文本"）
    std::string getMsgStatusDesc() const;      // 获取消息状态描述（如"已读"）

private:
    // 成员变量（驼峰命名，与数据库字段一一对应）
    uint64_t id;               // 主键Id
    uint64_t msgId;            // 新增: 业务/第三方消息ID（可用于去重或跨系统关联）
    std::string sessionId;     // 新增: 会话ID（字符串，用于标识对话/convId等）
    std::string sendUserId;       // 发送方用户ID（0=系统）
    ReceiveType receiveType;   // 接收类型（单聊/群聊）
    std::string receiveId;        // 接收方ID（用户ID/群ID）
    MsgType msgType;           // 消息类型
    std::string msgContent;    // 消息内容
    MsgStatus msgStatus;       // 消息状态
    uint64_t sendTime;         // 发送时间（Unix 时间戳）
    uint64_t readTime;         // 读取时间（Unix 时间戳，0 表示未读）
    std::string extendInfo;    // 扩展信息（JSON格式）
};

#endif // CHAT_RECORD_H