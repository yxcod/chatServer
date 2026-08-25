#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "UserLoginService.h"
#include "LoginDao.h"
#include "UserInfoDao.h"

namespace
{
// Universal character names keep these UTF-8 defaults independent of the
// source file's Windows code page.
constexpr const char* kDefaultNicknamePrefix =
    u8"\u9ED8\u8BA4\u6635\u79F0";
constexpr const char* kDefaultSignature =
    u8"\u9ED8\u8BA4\u7B7E\u540D";
constexpr const char* kDefaultRegion =
    u8"\u4E2D\u56FD\u5317\u4EAC";
}

// 初始化密码加密工具和 JWT 工具；JWT 密钥当前按项目要求固定在代码中。
UserLoginService::UserLoginService()
{
	encryptUtil = std::make_shared<OpenSslEncryptUtil>();
	// 固定密钥可避免重启后旧 Token 失效，但生产环境应改回安全的外部配置。
	constexpr const char* jwtSecret = "c9bb708f526d420ea88d83cd316d662921646869efaf425eb150ab99d20f48bc";
	tokenUtil = std::make_shared<JwtTokenUtil>(jwtSecret);
}
Json::Value UserLoginService::registerUser(const std::string& account, const std::string& password)
{
	LoginDao loginDao;
	Json::Value returnJson;
	//娉ㄥ唽涓嶈繑鍥瀟oken浣嗘槸涓轰簡鍜岀櫥褰曡蛋鍚屼竴鎺ュ彛杩欐牱闄勪笂榛樿鍊?
	returnJson["code"] = 101;
	returnJson["token"] = "";
	//鐢熸垚闅忔満鐩?
	if (encryptUtil)
	{
		std::string salt = encryptUtil->generateSalt();
		std::string newPassword = encryptUtil->sha256Encrypt(password, salt);
		if (loginDao.registerAccount(account, newPassword, salt) > 0)
		{
			
			UserInfo userInfo;
			UserInfoDao userInfoDao;
			userInfo.setUserAccount(account);
			std::string nickName =
				std::string(kDefaultNicknamePrefix) + account.substr(0, 5);
			userInfo.setNickName(nickName);
			userInfo.setAvatar("init");
			const auto currentTime = Logger::GetInstance().getcurrentTime();
			userInfo.setCreateTime(currentTime);
			userInfo.setModifyTime(currentTime);
			userInfo.setGender(1);
			userInfo.setRegion(kDefaultRegion);
			userInfo.setSignature(kDefaultSignature);
			userInfo.setState(1);
			if (userInfoDao.insertUserInfo(userInfo) > 0)
			{
				returnJson["code"] = 100;
				//娉ㄥ唽鎴愬姛鍚庡垱寤虹敤鎴锋暟鎹洰褰?
				Logger::GetInstance().createDataDirectories(account);
			}
			//鐢ㄦ埛鏁版嵁鏇存柊澶辫触
			else
			{
				returnJson["code"] = 103;
			}
			
			
		}

		return returnJson;
	}
	

}

Json::Value UserLoginService::login(const std::string& account, const std::string& password)
{
	// 100 鎴愬姛鐧婚檰 101 瀵嗙爜閿欒 102 鐢ㄦ埛涓嶅瓨鍦?103 鐢ㄦ埛琚皝绂?
	LoginDao loginDao;
	Json::Value returnJson;
	returnJson["code"] = 101;
	returnJson["token"] = "Erro";

	// 浣跨敤鏂扮殑 LoginInfo 缁撴瀯浣?
	LoginInfo info = loginDao.loginAccount(account);

	// 鐢ㄦ埛涓嶅瓨鍦?
	if (!info.found)
	{
		returnJson["code"] = 102;
		return returnJson;
	}

	// 鏍￠獙瀵嗙爜
	if (encryptUtil->verifyPassword(password, info.password, info.salt))
	{
		// 璐﹀彿琚皝绂?
		if (info.isBan == 1)
		{
			returnJson["code"] = 103;
		}
		else
		{
			returnJson["code"] = 100;
			// 鐢熸垚 token
			returnJson["token"] = tokenUtil->generateToken(account, "ios");
		}
	}
	else
	{
		// 瀵嗙爜閿欒锛屼繚鐣欓粯璁?101
		returnJson["code"] = 101;
	}

	return returnJson;
	
	
}
Json::Value UserLoginService::changePassword(const std::string& account,
	const std::string& oldPassword,
	const std::string& newPassword)
{
	LoginDao loginDao;
	Json::Value returnJson;
	// 100 鎴愬姛锛?01 鍘熷瘑鐮侀敊璇垨鏇存柊澶辫触锛?02 鐢ㄦ埛涓嶅瓨鍦?
	returnJson["code"] = 101;

	// 浣跨敤 LoginInfo 鏇夸唬 sql::ResultSet*
	LoginInfo info = loginDao.loginAccount(account);

	// 鐢ㄦ埛涓嶅瓨鍦?
	if (!info.found)
	{
		returnJson["code"] = 102;
		return returnJson;
	}

	// 鏍￠獙鏃у瘑鐮?
	if (encryptUtil->verifyPassword(oldPassword, info.password, info.salt))
	{
		// 鐢熸垚鏂板瘑鐮佹暎鍒?
		std::string newHashedPwd = encryptUtil->sha256Encrypt(newPassword, info.salt);

		// 淇敼瀵嗙爜
		if (loginDao.changePassword(account, newHashedPwd) > 0)
		{
			returnJson["code"] = 100;
		}
		// 鍚﹀垯淇濇寔 101
	}
	else
	{
		// 鍘熷瘑鐮侀敊璇紝淇濇寔 101
		returnJson["code"] = 101;
	}

	return returnJson;
}

Json::Value UserLoginService::resetPassword(
	const std::string& account,
	const std::string& newPassword)
{
	LoginDao loginDao;
	Json::Value returnJson;
	returnJson["code"] = 101;

	const LoginInfo info = loginDao.loginAccount(account);
	if (!info.found)
	{
		returnJson["code"] = 102;
		return returnJson;
	}

	const std::string encryptedPassword =
		encryptUtil->sha256Encrypt(newPassword, info.salt);
	if (loginDao.changePassword(account, encryptedPassword) > 0)
	{
		returnJson["code"] = 100;
	}

	return returnJson;
}
