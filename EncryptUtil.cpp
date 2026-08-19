#include "EncryptUtil.h"
#include <openssl/sha.h>   // SHA256 加密
#include <openssl/md5.h>   // MD5 加密
#include <openssl/rand.h>  // 随机数生成（用于盐值）
#include <stdexcept>
#include <cstring>

/**
 * 二进制转十六进制（静态辅助函数）
 */
std::string EncryptUtil::binToHex(const unsigned char* data, size_t length) {
    std::string hexStr;
    hexStr.reserve(length * 2);  // 预分配内存，提高效率
    const char* hexChars = "0123456789abcdef";

    for (size_t i = 0; i < length; ++i) {
        unsigned char byte = data[i];
        hexStr += hexChars[byte >> 4];  // 高4位
        hexStr += hexChars[byte & 0x0F];// 低4位
    }
    return hexStr;
}

/**
 * SHA256 加密实现（支持加盐）
 */
std::string OpenSslEncryptUtil::sha256Encrypt(const std::string& plainText, const std::string& salt) const {
	std::string data = plainText + salt;

	// 1. 创建加密上下文（OpenSSL 3.0+ 推荐用 EVP_MD_CTX_new，替代旧版 SHA256_CTX）
	EVP_MD_CTX* ctx = EVP_MD_CTX_new();
	if (!ctx) {
		throw std::runtime_error("Failed to create EVP context for SHA256");
	}

	try {
		// 2. 初始化上下文：指定算法为 SHA256（EVP_sha256() 是 3.0+ 标准接口）
		if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
			throw std::runtime_error("SHA256 initialization failed");
		}

		// 3. 更新加密数据（明文 + 盐）
		if (EVP_DigestUpdate(ctx, data.c_str(), data.length()) != 1) {
			throw std::runtime_error("SHA256 update failed");
		}

		// 4. 最终化加密，获取结果（SHA256 结果长度固定 32 字节）
		unsigned char hash[EVP_MAX_MD_SIZE]; // EVP_MAX_MD_SIZE 是 OpenSSL 定义的最大哈希长度
		unsigned int hashLen = 0;
		if (EVP_DigestFinal_ex(ctx, hash, &hashLen) != 1) {
			throw std::runtime_error("SHA256 finalization failed");
		}

		// 验证结果长度是否为 SHA256 标准长度（32 字节）
		if (hashLen != EVP_MD_size(EVP_sha256())) {
			throw std::runtime_error("SHA256 hash length mismatch");
		}

		// 5. 二进制转十六进制
		std::string result = EncryptUtil::binToHex(hash, hashLen);
		EVP_MD_CTX_free(ctx); // 释放上下文（必须在 return 前）
		return result;

	}
	catch (...) {
		EVP_MD_CTX_free(ctx); // 异常时也要释放上下文，避免内存泄漏
		throw;
	}
}

/**
 * 生成随机盐值（基于 OpenSSL 安全随机数）
 */
std::string OpenSslEncryptUtil::generateSalt(size_t length) const {
    if (length == 0) {
        throw std::invalid_argument("Salt length cannot be zero");
    }

    // 盐值字符集：大小写字母 + 数字（62个字符，避免特殊字符）
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::string salt;
    salt.reserve(length);

    // 生成 length 个随机索引，从字符集中选取字符
    for (size_t i = 0; i < length; ++i) {
        unsigned char randByte;
        // 生成 1 字节安全随机数（OpenSSL RAND_bytes 是密码学安全的随机数生成器）
        if (RAND_bytes(&randByte, 1) != 1) {
            throw std::runtime_error("Failed to generate secure random salt");
        }
        // 取随机数的低 6 位（0-61），对应字符集索引
        size_t idx = randByte % chars.length();
        salt += chars[idx];
    }

    return salt;
}

/**
 * MD5 加密实现（非敏感数据用）
 */
std::string OpenSslEncryptUtil::md5Encrypt(const std::string& plainText) const {
	// 1. 创建加密上下文
	EVP_MD_CTX* ctx = EVP_MD_CTX_new();
	if (!ctx) {
		throw std::runtime_error("Failed to create EVP context for MD5");
	}

	try {
		// 2. 初始化上下文：指定算法为 MD5
		if (EVP_DigestInit_ex(ctx, EVP_md5(), nullptr) != 1) {
			throw std::runtime_error("MD5 initialization failed");
		}

		// 3. 更新加密数据
		if (EVP_DigestUpdate(ctx, plainText.c_str(), plainText.length()) != 1) {
			throw std::runtime_error("MD5 update failed");
		}

		// 4. 最终化加密
		unsigned char hash[EVP_MAX_MD_SIZE];
		unsigned int hashLen = 0;
		if (EVP_DigestFinal_ex(ctx, hash, &hashLen) != 1) {
			throw std::runtime_error("MD5 finalization failed");
		}

		// 验证 MD5 结果长度（16 字节）
		if (hashLen != EVP_MD_size(EVP_md5())) {
			throw std::runtime_error("MD5 hash length mismatch");
		}

		// 5. 二进制转十六进制
		std::string result = EncryptUtil::binToHex(hash, hashLen);
		EVP_MD_CTX_free(ctx);
		return result;

	}
	catch (...) {
		EVP_MD_CTX_free(ctx);
		throw;
	}
}

bool OpenSslEncryptUtil::verifyPassword(const std::string& inputPwd, const std::string& dbEncryptedPwd, const std::string& salt) const
{
	// 1. 明文密码 + 盐（盐是数据库中存储的随机字符串，每个用户唯一）
	std::string pwdWithSalt = inputPwd + salt;
	// 2. SHA256 加密（EncryptUtil 封装加密逻辑，Service 层不关心具体加密算法）
	std::string encryptedInputPwd = this->sha256Encrypt(pwdWithSalt);
	// 3. 对比加密后的密码与数据库存储的密码
	return encryptedInputPwd == dbEncryptedPwd;
}
