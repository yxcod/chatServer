#ifndef ENCRYPT_UTIL_H
#define ENCRYPT_UTIL_H

#include <string>

/**
 * 加密工具类接口（抽象基类）
 * 定义加密相关核心接口，子类实现具体算法
 */
class EncryptUtil {
public:
    virtual ~EncryptUtil() = default;

    /**
     * SHA256 加密（支持加盐）
     * @param plainText 明文（如用户密码）
     * @param salt 盐值（可选，默认空字符串）
     * @return 加密后的十六进制字符串（64位）
     */
    virtual std::string sha256Encrypt(const std::string& plainText, const std::string& salt = "") const = 0;

    /**
     * 生成随机盐值（用于密码加密，每个用户唯一）
     * @param length 盐值长度（默认16位，推荐16-32位）
     * @return 随机盐值字符串（包含大小写字母、数字）
     */
    virtual std::string generateSalt(size_t length = 16) const = 0;

    /**
     * MD5 加密（用于非敏感数据，如日志脱敏、缓存key生成）
     * @param plainText 明文
     * @return 加密后的十六进制字符串（32位）
     */
    virtual std::string md5Encrypt(const std::string& plainText) const = 0;

	// 校验密码（明文密码加密后与数据库存储的加密密码对比）
    virtual bool verifyPassword(const std::string& inputPwd, const std::string& dbEncryptedPwd, const std::string& salt) const = 0;

protected:
    /**
     * 辅助函数：将二进制数据转为十六进制字符串（内部使用）
     * @param data 二进制数据
     * @param length 数据长度
     * @return 十六进制字符串（小写）
     */
    static std::string binToHex(const unsigned char* data, size_t length);
};

/**
 * OpenSSL 实现的加密工具类（具体实现）
 */
class OpenSslEncryptUtil : public EncryptUtil {
public:
    std::string sha256Encrypt(const std::string& plainText, const std::string& salt = "") const override;
    std::string generateSalt(size_t length = 16) const override;
    std::string md5Encrypt(const std::string& plainText) const override;
    bool verifyPassword(const std::string& inputPwd, const std::string& dbEncryptedPwd, const std::string& salt) const override;
};

#endif // ENCRYPT_UTIL_H