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
		ADD_METHOD_TO(FilesController::loadVideo, "/api/video/download", Get);
		ADD_METHOD_TO(FilesController::upLoadVideo, "/api/video/upload", Post);
		ADD_METHOD_TO(FilesController::loadAudio, "/api/audio/download", Get);
		ADD_METHOD_TO(FilesController::upLoadAudio, "/api/audio/upload", Post);
		ADD_METHOD_TO(FilesController::loadChatFile, "/api/file/download", Get);
		ADD_METHOD_TO(FilesController::upLoadChatFile, "/api/file/upload", Post);
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

		// 视频上传和支持 HTTP Range 的在线播放/下载。
		void loadVideo(const HttpRequestPtr& req,
			std::function<void(const HttpResponsePtr&)>&& callback);
		void upLoadVideo(const HttpRequestPtr& req,
			std::function<void(const HttpResponsePtr&)>&& callback);

		// 聊天语音上传和支持 HTTP Range 的在线播放。
		void loadAudio(const HttpRequestPtr& req,
			std::function<void(const HttpResponsePtr&)>&& callback);
		void upLoadAudio(const HttpRequestPtr& req,
			std::function<void(const HttpResponsePtr&)>&& callback);

		// 聊天附件上传/下载，单个文件最大 300 MB。
		void loadChatFile(const HttpRequestPtr& req,
			std::function<void(const HttpResponsePtr&)>&& callback);
		void upLoadChatFile(const HttpRequestPtr& req,
			std::function<void(const HttpResponsePtr&)>&& callback);
	};

}// namespace api
