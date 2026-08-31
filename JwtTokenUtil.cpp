#include "JwtTokenUtil.h"
#include "LoginDao.h"
#include <openssl/hmac.h>   // HMAC-SHA256 签名
#include <openssl/bio.h>    // Base64 编码解码
#include <openssl/evp.h>    // 加密算法上下文
#include <openssl/sha.h>  // 必须包含这行！
#include <openssl/md5.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <stdexcept>
#include <cstring>
#include <sstream>
#include <algorithm>

// 构造函数：初始化密钥和过期时间
JwtTokenUtil::JwtTokenUtil(std::string secretKey, uint64_t expireSeconds)
    : m_secretKey(std::move(secretKey)),
    m_expireSeconds(expireSeconds),
    m_algorithm("HS256") {
    // 校验密钥有效性（密钥为空或过短会导致签名不安全）
    if (m_secretKey.empty() || m_secretKey.length() < 16) {
        throw std::invalid_argument("JWT secret key must be at least 16 characters long");
    }
}

/**
 * 生成 JWT Token（通用方法，支持自定义载荷）
 */
std::string JwtTokenUtil::generateToken(const std::unordered_map<std::string, std::string>& payload) {
    // 步骤1：构建 Header（固定格式，声明算法和 Token 类型）
    std::string header = R"({"alg":")" + m_algorithm + R"(","typ":"JWT"})";
    std::string encodedHeader = base64UrlEncode(header);

    // 步骤2：构建 Payload（自定义数据 + 过期时间）
    std::ostringstream payloadStream;
    payloadStream << "{";

    // 添加自定义载荷（如 userId、deviceId 等）
    for (auto it = payload.begin(); it != payload.end(); ++it) {
        if (it != payload.begin()) {
            payloadStream << ",";
        }
        payloadStream << "\"" << it->first << "\":\"" << it->second << "\"";
    }

    // 添加过期时间（exp：当前时间戳 + 过期秒数）
    auto now = std::chrono::system_clock::now();
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    uint64_t exp = timestamp + m_expireSeconds;
    payloadStream << ",\"exp\":\"" << exp << "\"";

    payloadStream << "}";
    std::string payloadStr = payloadStream.str();
    std::string encodedPayload = base64UrlEncode(payloadStr);

    // 步骤3：生成签名（Header.Payload + 密钥 → HMAC-SHA256 签名）
    std::string data = encodedHeader + "." + encodedPayload;
    std::string signature = hmacSha256Sign(data);

    // 步骤4：组装 Token（Header.Payload.Signature）
    return encodedHeader + "." + encodedPayload + "." + signature;
}

/**
 * 重载：生成 Token（登录场景专用，直接传 userId 和 deviceId）
 */
std::string JwtTokenUtil::generateToken(std::string userId, const std::string& deviceId) {
    std::unordered_map<std::string, std::string> payload;
    payload["userId"] = userId;
    if (!deviceId.empty()) {
        payload["deviceId"] = deviceId;
    }
    return generateToken(payload);
}

/**
 * 验证 Token 有效性（签名正确 + 未过期）后端无需保存token拿到前端返回的token可以直接判断是否正确
 * JWT 的verifyToken只验证 “Token 是否是后端签发、未篡改、未过期”，但不验证
 * “当前请求者是否是 Token 的真正所有者”—— 这需要你在业务层补充 “权限绑定校验”，而不是依赖 Token 本身。
 * 
 */
bool JwtTokenUtil::verifyToken(const std::string& token) {
    // 分割 Token 为 Header.Payload.Signature（必须包含两个 "."）
    size_t firstDot = token.find('.');
    size_t secondDot = token.find('.', firstDot + 1);
    if (firstDot == std::string::npos || secondDot == std::string::npos || secondDot == token.length() - 1) {
        return false; // Token 格式错误
    }

    // 提取三部分
    std::string encodedHeader = token.substr(0, firstDot);
    std::string encodedPayload = token.substr(firstDot + 1, secondDot - firstDot - 1);
    std::string signature = token.substr(secondDot + 1);

    // 步骤1：验证签名
    std::string data = encodedHeader + "." + encodedPayload;
    if (!verifySignature(data, signature)) {
        return false; // 签名错误
    }

    // 步骤2：解析 Payload，检查过期时间
    std::string payloadStr = base64UrlDecode(encodedPayload);
    if (payloadStr.empty()) {
        return false; // Payload 解码失败
    }

    // 提取 exp 字段（简单 JSON 解析，适用于固定格式的 Payload）
    size_t expStart = payloadStr.find("\"exp\":\"");
    size_t expEnd = payloadStr.find("\"", expStart + 7);
    if (expStart == std::string::npos || expEnd == std::string::npos) {
        return false; // 缺少 exp 字段
    }

    std::string exp = payloadStr.substr(expStart + 7, expEnd - (expStart + 7));
    if (!isTokenNotExpired(exp)) return false;

    // A deleted or banned account invalidates every previously issued token.
    const std::string userMarker = "\"userId\":\"";
    const size_t userStart = payloadStr.find(userMarker);
    const size_t userEnd = userStart == std::string::npos
        ? std::string::npos
        : payloadStr.find('"', userStart + userMarker.size());
    if (userStart == std::string::npos || userEnd == std::string::npos)
        return false;
    const std::string userName = payloadStr.substr(
        userStart + userMarker.size(),
        userEnd - (userStart + userMarker.size()));
    return LoginDao().isAccountActive(userName);
}

/**
 * 解析 Token 中的载荷数据
 */
std::unordered_map<std::string, std::string> JwtTokenUtil::parsePayload(const std::string& token) {
    std::unordered_map<std::string, std::string> payload;

    // 先验证 Token 有效性（无效则返回空）
    if (!verifyToken(token)) {
        return payload;
    }

    // 分割并解码 Payload
    size_t firstDot = token.find('.');
    size_t secondDot = token.find('.', firstDot + 1);
    std::string encodedPayload = token.substr(firstDot + 1, secondDot - firstDot - 1);
    std::string payloadStr = base64UrlDecode(encodedPayload);
    if (payloadStr.empty()) {
        return payload;
    }

    // 简单 JSON 解析（适用于 key-value 均为字符串的场景，无需依赖 JSON 库）
    // 移除前后 {}
    if (payloadStr.front() == '{' && payloadStr.back() == '}') {
        payloadStr = payloadStr.substr(1, payloadStr.length() - 2);
    }

    // 分割键值对（按 , 分割）
    std::stringstream ss(payloadStr);
    std::string pair;
    while (std::getline(ss, pair, ',')) {
        // 分割 key 和 value（按 : 分割）
        size_t colon = pair.find(':');
        if (colon == std::string::npos) {
            continue;
        }

        // 提取 key（去除前后 "）
        std::string key = pair.substr(0, colon);
        key.erase(std::remove(key.begin(), key.end(), '"'), key.end());

        // 提取 value（去除前后 "）
        std::string value = pair.substr(colon + 1);
        value.erase(std::remove(value.begin(), value.end(), '"'), value.end());

        payload[key] = value;
    }

    return payload;
}

/**
 * 快捷方法：从 Token 中解析用户ID
 */
uint64_t JwtTokenUtil::parseUserIdFromToken(const std::string& token) {
    auto payload = parsePayload(token);
    auto it = payload.find("userId");
    if (it == payload.end()) {
        return 0;
    }
    try {
        return std::stoull(it->second);
    }
    catch (...) {
        return 0;
    }
}

std::optional<std::string> JwtTokenUtil::extractBearerToken(const HttpRequestPtr& req)
{
	// 1. 获取 Authorization 头
	auto authHeader = req->getHeader("Authorization");
	if (authHeader.empty()) {
		return std::nullopt; // 头不存在
	}

	// 2. 正则匹配 Bearer Token 格式（忽略大小写，支持空格分隔）
    std::regex pattern(R"(^Bearer\s+(.+)$)", std::regex_constants::icase);
    std::smatch matchResult;
	if (!regex_match(authHeader, matchResult, pattern)) {
		return std::nullopt; // 格式错误（不是 Bearer 类型）
	}

	// 3. 提取匹配到的 token（第一个子匹配项）
	if (matchResult.size() < 2) {
		return std::nullopt;
	}
	return matchResult[1].str();
}

/**
 * Base64 URL 编码（JWT 专用）
 * 标准 Base64 替换规则：+ → -, / → _, = → 去除
 */
std::string JwtTokenUtil::base64UrlEncode(const std::string& data) {
    BIO* bio = BIO_new(BIO_f_base64());
    BIO* mem = BIO_new(BIO_s_mem());
    bio = BIO_push(bio, mem);

    // 禁用 Base64 换行符（默认每 64 字符换行，JWT 不允许）
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    // 写入数据并刷新
    BIO_write(bio, data.c_str(), static_cast<int>(data.length()));
    BIO_flush(bio);

    // 读取编码结果
    char* buf = nullptr;
    long len = BIO_get_mem_data(bio, &buf);
    std::string encoded(buf, len);

    // 释放资源
    BIO_free_all(bio);

    // 替换字符：+ → -, / → _, 去除 =
    std::replace(encoded.begin(), encoded.end(), '+', '-');
    std::replace(encoded.begin(), encoded.end(), '/', '_');
    encoded.erase(std::remove(encoded.begin(), encoded.end(), '='), encoded.end());

    return encoded;
}

/**
 * Base64 URL 解码
 */
std::string JwtTokenUtil::base64UrlDecode(const std::string& data) {
    // 还原字符：- → +, _ → /, 补充 = 使长度为 4 的倍数
    std::string decodedData = data;
    std::replace(decodedData.begin(), decodedData.end(), '-', '+');
    std::replace(decodedData.begin(), decodedData.end(), '_', '/');

    // 补充 =（Base64 解码要求长度为 4 的倍数）
    size_t remainder = decodedData.length() % 4;
    if (remainder != 0) {
        decodedData.append(4 - remainder, '=');
    }

    BIO* base64 = BIO_new(BIO_f_base64());
    BIO* memory = BIO_new_mem_buf(
        decodedData.data(), static_cast<int>(decodedData.size()));
    if (base64 == nullptr || memory == nullptr) {
        if (base64 != nullptr) BIO_free(base64);
        if (memory != nullptr) BIO_free(memory);
        return {};
    }
    BIO_set_flags(base64, BIO_FLAGS_BASE64_NO_NL);
    BIO* chain = BIO_push(base64, memory);

    std::string result(decodedData.size(), '\0');
    const int length = BIO_read(
        chain, result.data(), static_cast<int>(result.size()));
    BIO_free_all(chain);
    if (length <= 0) {
        return {};
    }
    result.resize(static_cast<std::size_t>(length));
    return result;
}

/**
 * HMAC-SHA256 签名（返回 Base64 URL 编码结果）
 */
std::string JwtTokenUtil::hmacSha256Sign(const std::string& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    unsigned int hashLen = 0;

    // 生成 HMAC-SHA256 签名
    HMAC(EVP_sha256(),
        m_secretKey.c_str(), static_cast<int>(m_secretKey.length()),
        reinterpret_cast<const unsigned char*>(data.c_str()), data.length(),
        hash, &hashLen);

    // 签名结果转为字符串，再进行 Base64 URL 编码
    std::string signature(reinterpret_cast<char*>(hash), hashLen);
    return base64UrlEncode(signature);
}

/**
 * 验证签名（对比生成的签名与 Token 中的签名）
 */
bool JwtTokenUtil::verifySignature(const std::string& data, const std::string& signature) {
    // 用相同数据和密钥生成签名
    std::string expectedSignature = hmacSha256Sign(data);
    if (expectedSignature.size() != signature.size()) {
        return false;
    }
    // 对比签名（时间恒定比较，防止计时攻击）
    return CRYPTO_memcmp(expectedSignature.c_str(), signature.c_str(), expectedSignature.length()) == 0;
}

/**
 * 检查 Token 是否过期
 */
bool JwtTokenUtil::isTokenNotExpired(const std::string& exp) {
    try {
        // 解析 exp 为时间戳（秒）
        uint64_t expTimestamp = std::stoull(exp);
        // 获取当前时间戳
        auto now = std::chrono::system_clock::now();
        uint64_t currentTimestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        // 未过期 = 当前时间 < 过期时间
        return currentTimestamp < expTimestamp;
    }
    catch (...) {
        return false; // exp 格式错误，视为过期
    }
}
