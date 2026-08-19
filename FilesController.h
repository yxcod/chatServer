#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace api {
	class FilesController : public drogon::HttpController<FilesController> {
	public:
		METHOD_LIST_BEGIN
		ADD_METHOD_TO(FilesController::downloadFile, "/api/bigFile/download", Get);
		ADD_METHOD_TO(FilesController::upLoadFile, "/api/bigFile/upload", Post);
		ADD_METHOD_TO(FilesController::loadImage, "/api/image/download", Get);
		ADD_METHOD_TO(FilesController::upLoadImage, "/api/image/upload", Post);
		METHOD_LIST_END
		//文件下载
		void downloadFile(const HttpRequestPtr& req,
				std::function<void(const HttpResponsePtr&)>&& callback);
		//文件上传
		void upLoadFile(const HttpRequestPtr& req,
			std::function<void(const HttpResponsePtr&)>&& callback);

		//图片下载
		void loadImage(const HttpRequestPtr& req,
			std::function<void(const HttpResponsePtr&)>&& callback);
		// 图片上传：根据 userName 创建目录，按 imageName.JPG 保存
		void upLoadImage(const HttpRequestPtr& req,
			std::function<void(const HttpResponsePtr&)>&& callback);
	};

}// namespace api
