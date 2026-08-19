#include "FilesController.h"
#include "FileService.h"
#include <json/json.h>
#include<fstream>
namespace api {

	void FilesController::upLoadFile(const HttpRequestPtr& req,
		std::function<void(const HttpResponsePtr&)>&& callback) {
		auto contentType = req->getContentType();
		std::string request_file_root = "./ImageData";
		//户id
		std::string filename = req->getParameter("userName");
		if (contentType == CT_APPLICATION_OCTET_STREAM ||
			contentType == CT_TEXT_PLAIN ||
			contentType == CT_APPLICATION_JSON) {
			const auto& fileData = req->getBody();

			bool success = FileService::uploadFile(request_file_root, filename, std::string(fileData));

			Json::Value json;
			if (success) {
				json["code"] = 200;
				json["message"] = "File uploaded successfully";
			}
			else {
				json["code"] = 500;
				json["message"] = "Failed to write file";
			}

			auto resp = HttpResponse::newHttpJsonResponse(json);
			callback(resp);
			return;
		}

		if (contentType == CT_MULTIPART_FORM_DATA) {
			MultiPartParser parser;
			int result = parser.parse(req);
			if (result != 0) {
				Json::Value json;
				json["code"] = 400;
				json["message"] = "Failed to parse multipart/form-data request";
				auto resp = HttpResponse::newHttpJsonResponse(json);
				callback(resp);
				return;
			}

			const auto& files = parser.getFiles();
			if (files.empty()) {
				Json::Value json;
				json["code"] = 400;
				json["message"] = "No files uploaded";
				auto resp = HttpResponse::newHttpJsonResponse(json);
				callback(resp);
				return;
			}

			const auto& uploadedFile = files[0];
			std::string customPath = request_file_root + "/" + filename;
			int ret = uploadedFile.save(customPath);

			if (ret != 0) {
				Json::Value json;
				json["code"] = 500;
				json["message"] = "Failed to save file";
				auto resp = HttpResponse::newHttpJsonResponse(json);
				callback(resp);
				return;
			}

			Json::Value json;
			json["code"] = 200;
			json["message"] = "File uploaded successfully";
			auto resp = HttpResponse::newHttpJsonResponse(json);
			callback(resp);
			return;
		}

		Json::Value json;
		json["code"] = 400;
		json["message"] = "Unsupported Content-Type";
		auto resp = HttpResponse::newHttpJsonResponse(json);
		callback(resp);
	}



	void FilesController::downloadFile(const HttpRequestPtr& req,
		std::function<void(const HttpResponsePtr&)>&& callback) {

		auto fileDir = "./ImageData/" + req->getParameter("userid");
		auto fileName = req->getParameter("imageName");
		// 获取文件路径
		auto filePathOpt = FileService::getFilePath(fileDir, fileName);
		if (!filePathOpt.has_value()) {
			Json::Value json;
			json["code"] = 404;
			json["message"] = "File not found";
			auto resp = HttpResponse::newHttpJsonResponse(json);
			callback(resp);
			return;
		}

		// 获取文件大小
		auto filePath = filePathOpt.value();
		auto fileSize = std::filesystem::file_size(filePath);

		// 解析 Range 请求头
		auto rangeHeader = req->getHeader("Range");
		auto rangeOpt = FileService::parseRangeHeader(rangeHeader, fileSize);
		//判断rangeHeader格式(bytes=0-1023)是否规范和要获取的大小范围是否合法
		if (!rangeOpt.has_value()) {
			auto resp = HttpResponse::newHttpResponse(
				drogon::k416RequestedRangeNotSatisfiable,
				drogon::CT_TEXT_PLAIN);
			callback(resp);
			return;
		}

		auto [start, end] = rangeOpt.value();
		//若不带rangeOpt头则默认为完整下载
		if (rangeHeader.empty()) {
			// 返回完整文件：HTTP 200 状态码
			// 返回完整文件
			auto resp = HttpResponse::newFileResponse(filePath);
			resp->setContentTypeCode(drogon::CT_APPLICATION_OCTET_STREAM);
			resp->addHeader("Accept-Ranges", "bytes");
			callback(resp);
			return;
		}

		// 读取分片数据
		auto chunkOpt = FileService::readFileRange(fileDir, fileName, start, end);
		if (!chunkOpt.has_value()) {
			auto resp = HttpResponse::newHttpResponse(
				drogon::k404NotFound,
				drogon::CT_TEXT_PLAIN);
			callback(resp);
			return;
		}

		auto [data, totalSize] = chunkOpt.value();

		// 构造响应
		//构造分片响应：HTTP 206 状态码（部分内容成功）
		auto resp = HttpResponse::newHttpResponse(
			drogon::k206PartialContent,
			drogon::CT_APPLICATION_OCTET_STREAM);

		resp->addHeader("Content-Range",
			"bytes " + std::to_string(start) + "-" +
			std::to_string(end) + "/" + std::to_string(totalSize));
		resp->addHeader("Accept-Ranges", "bytes");
		resp->setBody(data);
		callback(resp);
	}
	//下载图片
	void FilesController::loadImage(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
	{
		auto fileDir = "./imageData/" + req->getParameter("userName");
		if (req->getParameter("userName") == "123456")
		{
			std::cout << "ss";

		}
		else if (req->getParameter("userName") == "18856617420")
		{
			std::cout << "ss";
		}
		auto fileName = req->getParameter("imageName");
		auto filePathOpt = FileService::getFilePath(fileDir, fileName);
		if (!filePathOpt.has_value()) {
			Json::Value json;
			json["code"] = 404;
			json["message"] = "File not found";
			auto resp = HttpResponse::newHttpJsonResponse(json);
			callback(resp);
			return;
		}
		auto filePath = filePathOpt.value();
		std::ifstream file(filePath, std::ios::binary);
		std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		auto resp = HttpResponse::newHttpResponse();
		resp->setStatusCode(k200OK);
		resp->setBody(std::string(buffer.begin(), buffer.end()));
		resp->setContentTypeCode(CT_IMAGE_JPG);
		callback(resp);


	}
	// - userName: 用户名
	// - imageName: 图片名（不含后缀）
	// - file: 图片二进制内容
	void FilesController::upLoadImage(const HttpRequestPtr& req,
		std::function<void(const HttpResponsePtr&)>&& callback)
	{
		Json::Value json;
		std::string request_file_root = "./ImageData";
		std::string filename = req->getParameter("userName");
		auto contentType = req->getContentType();
		//二进制六文件
		if (contentType == CT_MULTIPART_FORM_DATA) {
			MultiPartParser parser;
			int result = parser.parse(req);
			if (result != 0) {
				json["code"] = 101;
				json["message"] = "Failed to parse multipart/form-data request";
				auto resp = HttpResponse::newHttpJsonResponse(json);
				callback(resp);
				return;
			}

			const auto& files = parser.getFiles();
			if (files.empty()) {
				json["code"] = 102;
				json["message"] = "No files uploaded";
				auto resp = HttpResponse::newHttpJsonResponse(json);
				callback(resp);
				return;
			}

			const auto& uploadedFile = files[0];
			std::string customPath = request_file_root + "/" + filename;
			int ret = uploadedFile.save(customPath);

			if (ret != 0) {
				json["code"] = 103;
				json["message"] = "Failed to save file";
				auto resp = HttpResponse::newHttpJsonResponse(json);
				callback(resp);
				return;
			}


			json["code"] = 100;
			json["message"] = "File uploaded successfully";
			auto resp = HttpResponse::newHttpJsonResponse(json);
			callback(resp);
			return;
		}
		json["code"] = 104;
		json["message"] = "Unsupported Content-Type";
		auto resp = HttpResponse::newHttpJsonResponse(json);
		callback(resp);
	}
}
