#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "UserLoginService.h"
#include <cstdlib>
#include <stdexcept>
#include "LoginDao.h"
#include "UserInfoDao.h"
UserLoginService::UserLoginService()
{
	encryptUtil = std::make_shared<OpenSslEncryptUtil>();  // SHA256 鍔犲瘑
	const char* jwtSecret = std::getenv("CHATSERVER_JWT_SECRET");
	if (!jwtSecret || *jwtSecret == '\0')
	{
		throw std::runtime_error("Missing CHATSERVER_JWT_SECRET environment variable.");
	}
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
			//灏介噺涓嶈鍦ㄤ唬鐮佷腑濉啓涓枃杩欐牱鎻掑叆鏁版嵁搴撲細鍥犱负缂栫爜闂瀵艰嚧鏁版嵁搴撴彃鍏ュけ璐?濡傛灉闈炶鍦╒Scode閲岄潰鏇存敼璇峰皢鏂囦欢缂栫爜鏀逛负UTF-8 
			std::string nickName = "榛樿鏄电О" + account.substr(0, 5);
			userInfo.setNickName(nickName);
			userInfo.setAvatar("init");
			userInfo.setCreateTime(Logger::GetInstance().getcurrentTime());
			userInfo.setGender(1);
			userInfo.setSignature("榛樿绛惧悕");
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


