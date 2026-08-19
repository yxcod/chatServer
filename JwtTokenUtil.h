#ifndef JWT_TOKEN_UTIL_H
#define JWT_TOKEN_UTIL_H

#include <string>
#include <unordered_map>
#include <chrono>  // 时间相关
#include <drogon/HttpController.h>
#include <drogon/HttpController.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <regex>
using namespace drogon;
/**
 * JWT Token 工具类（基于 HS256 算法）
 * 功能：生成 Token、验证 Token 有效性、解析 Token 中的载荷数据
 */
class JwtTokenUtil {
public:
    /**
     * 构造函数
     * @param secretKey 签名密钥（必须保密，建议长度 ≥ 32 位）
     * @param expireSeconds Token 过期时间（单位：秒，默认 3600 秒 = 1 小时）
     */
    explicit JwtTokenUtil(std::string secretKey, uint64_t expireSeconds = 360000);
    ~JwtTokenUtil() = default;
    /**
     * 生成 JWT Token
     * @param payload 自定义载荷（如 userId、deviceId 等）
     * @return 完整的 JWT Token 字符串（格式：Header.Payload.Signature）
     * @throw std::runtime_error 生成失败（如密钥为空、编码失败）
     */
    std::string generateToken(const std::unordered_map<std::string, std::string>& payload);

    /**
     * 重载：生成 Token（适配登录场景，直接传入 userId 和 deviceId）
     * @param userId 用户ID
     * @param deviceId 设备ID（可选）
     * @return JWT Token 字符串
     */
    std::string generateToken(std::string userId, const std::string& deviceId = "");

    /**
     * 验证 Token 有效性
     * @param token 待验证的 Token 字符串
     * @return true = 有效（签名正确 + 未过期），false = 无效
     */
    bool verifyToken(const std::string& token);

    /**
     * 解析 Token 中的载荷数据
     * @param token 待解析的 Token
     * @return 载荷键值对（仅当 Token 有效时返回完整数据，无效时返回空）
     */
    std::unordered_map<std::string, std::string> parsePayload(const std::string& token);

    /**
     * 从 Token 中解析用户ID（登录场景快捷方法）
     * @param token 待解析的 Token
     * @return 用户ID（Token 无效时返回 0）
     */
    uint64_t parseUserIdFromToken(const std::string& token);
	//解析出Bearer Token
    std::optional<std::string> extractBearerToken(const HttpRequestPtr& req);

private:
    /**
     * 辅助函数：Base64 URL 编码（JWT 专用，替换标准 Base64 的 +/= 字符）
     * @param data 待编码的字符串
     * @return Base64 URL 编码后的字符串
     */
    std::string base64UrlEncode(const std::string& data);

    /**
     * 辅助函数：Base64 URL 解码
     * @param data 待解码的 Base64 URL 字符串
     * @return 解码后的原始字符串（解码失败返回空）
     */
    std::string base64UrlDecode(const std::string& data);

    /**
     * 辅助函数：HMAC-SHA256 签名（JWT 签名核心）
     * @param data 待签名的数据（Header.Base64UrlEncode + "." + Payload.Base64UrlEncode）
     * @return 签名后的 Base64 URL 编码字符串
     */
    std::string hmacSha256Sign(const std::string& data);

    /**
     * 辅助函数：验证签名
     * @param data 待验证的数据（Header.Payload）
     * @param signature 待验证的签名（Token 的第三部分）
     * @return true = 签名正确，false = 签名错误
     */
    bool verifySignature(const std::string& data, const std::string& signature);

    /**
     * 辅助函数：检查 Token 是否过期
     * @param exp 过期时间戳（Payload 中的 "exp" 字段）
     * @return true = 未过期，false = 已过期
     */
    bool isTokenNotExpired(const std::string& exp);

private:
    std::string m_secretKey;       // JWT 签名密钥（必须保密）
    uint64_t m_expireSeconds;      // Token 过期时间（秒）
    const std::string m_algorithm; // 加密算法（固定为 HS256）
};

#endif // JWT_TOKEN_UTIL_H