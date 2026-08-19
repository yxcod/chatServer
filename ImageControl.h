//#pragma once
//#include <drogon/HttpController.h>
//#include <drogon/HttpRequest.h>
//#include <drogon/HttpResponse.h>
//#include <filesystem>
//#include <fstream>
//#include <string>
//#include <vector>
//#include <stdexcept>
//#include <openssl/bio.h>
//#include <openssl/evp.h>
//#include <openssl/buffer.h>
//#include "Logger .h"
//
//using namespace drogon;
//namespace fs = std::filesystem;
//
//class ImageController : public HttpController<ImageController> {
//public:
//    METHOD_LIST_BEGIN
//    ADD_METHOD_TO(ImageController::uploadImage, "/api/image/upload", Post);
//    ADD_METHOD_TO(ImageController::downloadImage, "/api/image/download", Get);
//    ADD_METHOD_TO(ImageController::deleteImage, "/api/image/delete", Post);
//    ADD_METHOD_TO(ImageController::handleOptions, "/api/image/*", Options);
//
//    METHOD_LIST_END
//
//private:
//    // 配置参数（按需调整）
//    static constexpr const char* kStorageDir = "./imageData/";
//    static constexpr const char* kBaseUrl = "http://192.168.1.100:8080";
//    static constexpr size_t kMaxFileSizeMB = 5; // 建议5MB以内（Base64无分片压力小）
//    static constexpr size_t kMaxFileSizeBytes = kMaxFileSizeMB * 1024 * 1024;
//
//   
//    // 初始化存储目录（自动创建）
//    bool initStorageDir(const std::string& fileDir) const {
//        try {
//            if (!fs::exists(fileDir)) {
//                fs::create_directories(fileDir);
//                LOG_DEBUG << "[ImageController] 已创建存储目录：" << fileDir;
//            }
//            return true;
//        }
//        catch (const std::exception& e) {
//            LOG_ERROR << "[ImageController] 创建目录失败：" << e.what();
//            return false;
//        }
//    }
//
//    // 拼接文件路径
//    std::string getFileFullPath(const std::string& filePath, const std::string& fileName) const {
//        //+ "./jpg"
//        return filePath + "/" + fileName;
//    }
//
//    // Base64 解码（依赖 OpenSSL，你的项目已包含）
//    std::vector<char> base64Decode(const std::string& base64Str) const {
//        // 移除 Base64 中的换行符（前端可能添加）
//        std::string cleanBase64 = base64Str;
//        cleanBase64.erase(std::remove(cleanBase64.begin(), cleanBase64.end(), '\n'), cleanBase64.end());
//        cleanBase64.erase(std::remove(cleanBase64.begin(), cleanBase64.end(), '\r'), cleanBase64.end());
//
//        // OpenSSL Base64 解码
//        BIO* bio = BIO_new_mem_buf(cleanBase64.data(), cleanBase64.size());
//        BIO* b64 = BIO_new(BIO_f_base64());
//        bio = BIO_push(b64, bio);
//        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // 禁用换行符处理
//
//        // 预分配解码缓冲区（Base64 解码后体积约为原字符串的 3/4）
//        std::vector<char> decodedData(cleanBase64.size() * 3 / 4 + 1);
//        int decodedLen = BIO_read(bio, decodedData.data(), decodedData.size());
//        if (decodedLen < 0) {
//            BIO_free_all(bio);
//            throw std::runtime_error("Base64 解码失败");
//        }
//
//        decodedData.resize(decodedLen);
//        BIO_free_all(bio);
//        return decodedData;
//    }
//
//public:
//    /**
//     * 图片上传接口（JSON + Base64）
//     * 请求体：{ "imageBase64": "图片的Base64字符串" }
//     * 响应体：{ "code": 0/1, "msg": "描述", "data": { "imageUrl": "访问URL" } }
//     */
//    void uploadImage(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
//
//        //100-上传成功 101 传入参数错误缺少imageBase64 字段 102上传图片太大最多不能超过5M 103服务器磁盘不足无法保存
//        Json::Value response;
//        response["code"] = 0;
//        try {
//            // 2. 解析 JSON 请求体
//            auto jsonPtr = req->getJsonObject();
//            if (!jsonPtr || !(*jsonPtr).isMember("imageBase64")) {
//                //"缺少 imageBase64 字段（请传入 Base64 编码的图片）";
//                response["code"] = 101;
//                auto resp = HttpResponse::newHttpJsonResponse(response);
//                setCorsHeaders(resp);
//                callback(resp);
//                return;
//            }
//
//            // 3. 获取 Base64 字符串并解码
//            std::string base64Str = (*jsonPtr)["imageBase64"].asString();
//            std::string userId = (*jsonPtr)["userId"].asString();
//            std::string imageName = (*jsonPtr)["imageName"].asString();
//            //更具用户账号开辟不同文件夹
//            std::string userFileDir = kStorageDir + userId;
//            std::vector<char> imageData = base64Decode(base64Str);
//            // 1. 初始化目录
//            if (!initStorageDir(userFileDir)) {
//                auto resp = HttpResponse::newHttpJsonResponse(response);
//                setCorsHeaders(resp);
//                callback(resp);
//                return;
//            }
//            // 4. 校验文件大小
//            if (imageData.size() > kMaxFileSizeBytes) {
//                //"文件超出限制 "
//                response["code"] = 102;
//                auto resp = HttpResponse::newHttpJsonResponse(response);
//                setCorsHeaders(resp);
//                callback(resp);
//                return;
//            }
//
//            // 5. 保存图片到服务器
//            std::string filePath = getFileFullPath(userFileDir, imageName);
//
//            std::ofstream outFile(filePath, std::ios::binary);
//            if (!outFile.is_open()) {
//                //"图片保存失败（权限不足或磁盘已满）";
//                response["code"] = 103;
//                auto resp = HttpResponse::newHttpJsonResponse(response);
//                setCorsHeaders(resp);
//                callback(resp);
//                return;
//            }
//            outFile.write(imageData.data(), imageData.size());
//            outFile.close();
//
//            // 6. 返回访问 URL
//            std::string imageUrl = std::string(kBaseUrl) + "/api/image/download?file=" + userFileDir;
//            //"上传成功";
//            response["code"] = 100;
//            response["imageUrl"]= imageUrl;
//
//            LOG_DEBUG << "[ImageController] 上传成功：" << filePath << "，大小：" << imageData.size() << "字节";
//
//        }
//        catch (const std::exception& e) {
//            LOG_ERROR << "[ImageController] 上传异常：" << e.what();
//        }
//
//        auto resp = HttpResponse::newHttpJsonResponse(response);
//        setCorsHeaders(resp);
//        callback(resp);
//    }
//
//    /**
//     * 图片下载接口（无改动，Drogon 1.9 原生支持）
//     */
//    void downloadImage(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
//        std::string imageName = req->getParameter("file");
//        std::string userId = req->getParameter("userId");
//        if (imageName.empty()) {
//            Json::Value errResp;
//            // "缺少文件名参数（格式：/download?file=xxx.jpg）";
//            errResp["code"] = 101;
//            auto resp = HttpResponse::newHttpJsonResponse(errResp);
//            setCorsHeaders(resp);
//            callback(resp);
//            return;
//        }
//        
//		//根据用户名加上图片名去读取图片 
//		std::string userFileDir = kStorageDir + userId;
//        std::string filePath = getFileFullPath(userFileDir, imageName);
//        if (!fs::exists(filePath) || !fs::is_regular_file(filePath)) {
//            Json::Value errResp;
//            //"图片不存在"
//            errResp["code"] = 102;
//            auto resp = HttpResponse::newHttpJsonResponse(errResp);
//            setCorsHeaders(resp);
//            callback(resp);
//            return;
//        }
//
//        try {
//            std::ifstream inFile(filePath, std::ios::binary | std::ios::ate);
//            if (!inFile.is_open()) throw std::runtime_error("文件读取失败（权限不足）");
//
//            std::streamsize fileSize = inFile.tellg();
//            inFile.seekg(0, std::ios::beg);
//            std::vector<char> imageData(fileSize);
//            inFile.read(imageData.data(), fileSize);
//            inFile.close();
//
//            // 返回 JPEG 二进制流
//            auto resp = HttpResponse::newHttpResponse();
//            resp->setStatusCode(k200OK);
//            resp->setContentTypeCode(ContentType::CT_IMAGE_JPG);
//            resp->setBody(std::string(imageData.begin(), imageData.end()));
//            setCorsHeaders(resp);
//
//            LOG_DEBUG << "[ImageController] 下载成功：" << filePath;
//            callback(resp);
//
//        }
//        catch (const std::exception& e) {
//            Json::Value errResp;
//            errResp["code"] = 1;
//            errResp["msg"] = "下载异常：" + std::string(e.what());
//            auto resp = HttpResponse::newHttpJsonResponse(errResp);
//            setCorsHeaders(resp);
//            LOG_ERROR << "[ImageController] 下载异常：" << e.what();
//            callback(resp);
//        }
//    }
//	void deleteImage(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
//		Json::Value response;
//		response["code"] = 1;
//		response["msg"] = "删除失败";
//
//		try {
//			auto jsonPtr = req->getJsonObject();
//			if (!jsonPtr || !(*jsonPtr).isMember("fileName")) {
//				response["msg"] = "缺少 fileName 字段";
//				auto resp = HttpResponse::newHttpJsonResponse(response);
//				setCorsHeaders(resp);
//				callback(resp);
//				return;
//			}
//
//			std::string userId = (*jsonPtr)["userId"].asString();
//			std::string imageName = (*jsonPtr)["imageName"].asString();
//			//根据用户名加上图片名去读取图片 
//			std::string userFileDir = kStorageDir + userId;
//			std::string filePath = getFileFullPath(userFileDir, imageName);
//			
//
//			if (fs::exists(filePath)) {
//				fs::remove(filePath); // 删除文件
//				response["code"] = 0;
//				response["msg"] = "删除成功";
//				LOG_DEBUG << "[ImageController] 删除图片：" << filePath;
//			}
//			else {
//				response["msg"] = "图片不存在";
//			}
//
//		}
//		catch (const std::exception& e) {
//			response["msg"] = "删除异常：" + std::string(e.what());
//			LOG_ERROR << "[ImageController] 删除异常：" << e.what();
//		}
//
//		auto resp = HttpResponse::newHttpJsonResponse(response);
//		setCorsHeaders(resp);
//		callback(resp);
//	}
//
//    /**
//     * 跨域预检接口（解决前端跨域问题）
//     */
//    void handleOptions(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
//        auto resp = HttpResponse::newHttpResponse();
//        resp->setStatusCode(k200OK);
//        setCorsHeaders(resp);
//        callback(resp);
//    }
//
//private:
//    // 设置跨域响应头
//    void setCorsHeaders(const HttpResponsePtr& resp) const {
//        resp->addHeader("Access-Control-Allow-Origin", "*");
//        resp->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
//        resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
//        resp->addHeader("Access-Control-Max-Age", "86400");
//    }
//};