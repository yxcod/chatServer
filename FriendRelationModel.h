#ifndef FRIEND_RELATION_H
#define FRIEND_RELATION_H

#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>

/**
 * 好友关系模型（对应 MySQL friend_relation 表）
 * 字段映射：
 * id           → BIGINT UNSIGNED → uint64_t
 * fromUserId   → BIGINT UNSIGNED → uint64_t（发起方用户ID）
 * toUserId     → BIGINT UNSIGNED → uint64_t（接收方用户ID）
 * status       → TINYINT         → uint8_t（0=待验证，1=已通过，2=已拒绝，3=已拉黑）
 * fromRemark   → VARCHAR(50)     → std::string（发起方对接收方的备注）
 * toRemark     → VARCHAR(50)     → std::string（接收方对发起方的备注）
 * source       → VARCHAR(30)     → std::string（好友来源）
 * applyMsg     → VARCHAR(200)    → std::string（申请消息）
 * createTime   → DATETIME        → std::tm（申请时间）
 * updateTime   → DATETIME        → std::tm（状态更新时间）
 */
class FriendRelation {
public:
    // 好友状态枚举（与数据库 status 字段对应，提高代码可读性）
    enum class RelationStatus : uint8_t {
        PENDING = 0,    // 待验证
        ACCEPTED = 1,   // 已通过（好友）
        REJECTED = 2,   // 已拒绝
        BLOCKED = 3,     // 已拉黑
		HASREAD = 4 ,     //已读
		HASDEL = 5      //已删除
    };

    // 1. 默认构造函数
    FriendRelation();

    // 2. 带参数构造函数（快速初始化所有字段）
    FriendRelation(uint64_t id, std::string fromUserId, std::string toUserId,
        RelationStatus status, const std::string& fromRemark,
        const std::string& toRemark, const std::string& source,
        const std::string& applyMsg, const uint64_t& createTime,
        const uint64_t& updateTime);

    // 3. 拷贝构造函数
    FriendRelation(const FriendRelation& other);

    // 4. 赋值运算符重载
    FriendRelation& operator=(const FriendRelation& other);

    // 5. 析构函数
    ~FriendRelation() = default;

    // -------------------------- Getter 函数 --------------------------
    uint64_t getId() const;
    std::string getFromUserId() const;
    std::string getToUserId() const;
    RelationStatus getStatus() const;          // 返回枚举类型，更直观
    uint8_t getStatusAsUInt8() const;          // 辅助：返回uint8_t类型（数据库存储用）
    const std::string& getFromRemark() const;  // 发起方→接收方的备注
    const std::string& getToRemark() const;    // 接收方→发起方的备注
    const std::string& getSource() const;
    const std::string& getApplyMsg() const;
    const uint64_t& getCreateTime() const;
    const uint64_t& getUpdateTime() const;

    // -------------------------- Setter 函数 --------------------------
    void setId(uint64_t id);
    void setFromUserId(std::string fromUserId);   // 校验：用户ID不能为0
    void setToUserId(std::string toUserId);       // 校验：用户ID不能为0，且不能与发起方ID相同
    void setStatus(RelationStatus status);     // 设置状态，自动更新updateTime
    void setStatus(uint8_t status);            // 重载：支持直接传入uint8_t（数据库读取用）
    void setFromRemark(const std::string& fromRemark);  // 校验长度≤50
    void setToRemark(const std::string& toRemark);      // 校验长度≤50
    void setSource(const std::string& source);          // 校验长度≤30
    void setApplyMsg(const std::string& applyMsg);      // 校验长度≤200
    void setCreateTime(const uint64_t& createTime);
    void setCreateTimeNow();                   // 便捷：设置申请时间为当前时间
    void setUpdateTime(const uint64_t& updateTime);
    void setUpdateTimeNow();                   // 便捷：设置更新时间为当前时间

    // -------------------------- 辅助函数 --------------------------
    std::string toString() const;              // 对象转字符串（调试用）
    std::string getStatusDesc() const;         // 获取状态描述（如"已通过"）

private:
    // 成员变量（驼峰命名，与数据库字段一一对应）
    uint64_t id;               // 好友关系ID
    std::string fromUserId;       // 发起好友请求的用户ID
    std::string toUserId;         // 接收好友请求的用户ID
    RelationStatus status;     // 好友状态（枚举类型）
    std::string fromRemark;    // 发起方对接收方的备注
    std::string toRemark;      // 接收方对发起方的备注
    std::string source;        // 好友来源（如"搜索添加"）
    std::string applyMsg;      // 申请好友时的验证消息
    uint64_t createTime;        // 申请时间
    uint64_t updateTime;        // 状态更新时间
};

#endif // FRIEND_RELATION_H