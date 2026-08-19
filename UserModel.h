#ifndef USER_INFO_H
#define USER_INFO_H

#include <string>
#include <ctime>  // 用于时间类型（数据库 DATETIME 对应 C++ tm 结构体）
#include <sstream>
#include <iomanip>

/**
 * 用户信息模型（对应 MySQL userinfo 表）
 * 字段映射：
 * userId      → BIGINT UNSIGNED → uint64
 * userName    → VARCHAR(50)     → std::string
 * nickName    → VARCHAR(50)     → std::string
 * avatar      → VARCHAR(255)    → std::string
 * gender      → TINYINT         → uint8_t（0=未知，1=男，2=女，无符号8位整数）
 * signature   → VARCHAR(200)    → std::string
 * createTime  → bigint unsigned        → std::tm（C++ 标准时间结构体，兼容日期时间格式）
 */
class UserInfo {
public:
    // 1. 默认构造函数（初始化默认值）
    UserInfo();

    // 2. 带参数构造函数（快速初始化所有字段）
    UserInfo(uint64_t userId, const std::string& userAccount, const std::string& nickName,
        const std::string& avatar, const int& gender, const std::string& signature,
        const uint64_t& createTime, const int& state, const uint64_t& modifyTime);

    // 3. 拷贝构造函数（深拷贝字符串字段）
    UserInfo(const UserInfo& other);

    // 4. 赋值运算符重载（避免浅拷贝问题）
    UserInfo& operator=(const UserInfo& other);

    // 5. 析构函数（字符串字段无需手动释放，编译器自动处理）
    ~UserInfo() = default;

    // -------------------------- Getter 函数（获取字段值）--------------------------
    uint64_t getUserId() const;                // 获取用户ID
    const std::string& getUserAccount() const;    // 获取用户名（返回const引用，避免拷贝）
    const std::string& getNickName() const;    // 获取昵称
    const std::string& getAvatar() const;      // 获取头像URL
    int getGender() const;                 // 获取性别
    const std::string& getSignature() const;   // 获取个性签名
    const uint64_t& getCreateTime() const;      // 获取注册时间
	const uint64_t& getModifyTime() const; //获取最后修改时间
    const int& getState() const;
    // -------------------------- Setter 函数（设置字段值）--------------------------
    void setUserId(uint64_t userId);                   // 设置用户ID
    void setUserAccount(const std::string& userAccount);     // 设置用户名（传入const引用，避免拷贝）
    void setNickName(const std::string& nickName);     // 设置昵称
    void setAvatar(const std::string& avatar);         // 设置头像URL
    void setGender(uint8_t gender);                    // 设置性别（自动过滤非法值：仅0/1/2有效）
    void setSignature(const std::string& signature);   // 设置个性签名
    void setCreateTime(const std::uint64_t& createTime);     // 设置注册时间
	void setModifyTime(const uint64_t& modifyTime); //设置最后修改时间
    void setCreateTimeNow();                           // 便捷方法：设置注册时间为当前系统时间
    void setState(const uint8_t& gender);
    // 将对象转为字符串（调试用，格式：key=value）
    std::string toString() const;

private:
    // 成员变量（驼峰命名，与数据库字段一一对应）
    uint64_t userId;       // 用户唯一ID（对应数据库 userId）
    std::string userAccount;  // 用户名（对应数据库 userName）
    std::string nickName;  // 用户昵称（对应数据库 nickName）
    std::string avatar;    // 头像URL（对应数据库 avatar）
    int gender;        // 性别（0=未知，1=男，2=女，对应数据库 gender）
    std::string signature; // 个性签名（对应数据库 signature）
    uint64_t createTime;    // 注册时间（对应数据库 createTime）
    int state;   //是否在线
    uint64_t modifyTime; //最后修改时间

};  

#endif // USER_INFO_H