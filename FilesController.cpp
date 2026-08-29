#include "FilesController.h"
#include "FileService.h"
#include <json/json.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace {
	constexpr std::size_t kMaxImageBytes = 5 * 1024 * 1024;
	constexpr std::size_t kMaxVideoBytes = 300ULL * 1024 * 1024;
	constexpr std::size_t kMaxAudioBytes = 10ULL * 1024 * 1024;
	constexpr std::size_t kMaxChatFileBytes = 300ULL * 1024 * 1024;
	const std::filesystem::path kImageRoot = "./imageData";
	const std::filesystem::path kVideoRoot = "./videoData";
	const std::filesystem::path kAudioRoot = "./audioData";
	const std::filesystem::path kChatFileRoot = "./fileData";

	bool isSafePathSegment(const std::string& value) {
		if (value.empty() || value == "." || value == ".." || value.size() > 180) {
			return false;
		}
		return std::none_of(value.begin(), value.end(), [](unsigned char ch) {
			return ch < 0x20 || ch == 0x7f || ch == '/' || ch == '\\' || ch == ':';
		});
	}

	std::string lowercase(std::string value) {
		std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
			return static_cast<char>(std::tolower(ch));
		});
		return value;
	}

	std::string detectImageMime(std::string_view data) {
		if (data.size() >= 3 &&
			static_cast<unsigned char>(data[0]) == 0xff &&
			static_cast<unsigned char>(data[1]) == 0xd8 &&
			static_cast<unsigned char>(data[2]) == 0xff) {
			return "image/jpeg";
		}
		static constexpr unsigned char pngSignature[] = {
			0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a
		};
		if (data.size() >= sizeof(pngSignature) &&
			std::equal(std::begin(pngSignature), std::end(pngSignature),
				reinterpret_cast<const unsigned char*>(data.data()))) {
			return "image/png";
		}
		if (data.size() >= 12 && data.substr(0, 4) == "RIFF" && data.substr(8, 4) == "WEBP") {
			return "image/webp";
		}
		return {};
	}

	std::string detectVideoMime(const std::string& filename, std::string_view data) {
		const auto extension = lowercase(std::filesystem::path(filename).extension().string());
		if (data.size() < 12 || data.substr(4, 4) != "ftyp") return {};
		if (extension == ".mp4" || extension == ".m4v") return "video/mp4";
		if (extension == ".mov") return "video/quicktime";
		return {};
	}

	std::string detectAudioMime(const std::string& filename, std::string_view data) {
		const auto extension = lowercase(std::filesystem::path(filename).extension().string());
		if ((extension == ".m4a" || extension == ".mp4") &&
			data.size() >= 12 && data.substr(4, 4) == "ftyp") {
			return "audio/mp4";
		}
		return {};
	}

	drogon::HttpResponsePtr jsonResponse(int code,
		const std::string& message,
		drogon::HttpStatusCode status) {
		Json::Value json;
		json["code"] = code;
		json["message"] = message;
		auto response = drogon::HttpResponse::newHttpJsonResponse(json);
		response->setStatusCode(status);
		return response;
	}

	std::string makeEtag(const std::filesystem::path& path) {
		auto modified = std::filesystem::last_write_time(path).time_since_epoch().count();
		auto size = std::filesystem::file_size(path);
		std::ostringstream stream;
		stream << "\"" << std::hex << size << '-'
			<< static_cast<long long>(modified) << "\"";
		return stream.str();
	}

	void schedulePrivacyUploadCleanup(
		const drogon::HttpRequestPtr& request,
		const std::filesystem::path& destination) {
		if (request->getParameter("privacy") != "1") return;
		// 隐私媒体即使后续 WebSocket 投递失败，也必须有独立的清理兜底。
		drogon::app().getLoop()->runAfter(300, [destination]() {
			std::error_code ignored;
			std::filesystem::remove(destination, ignored);
		});
	}
}

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
		const auto ownerId = req->getParameter("userName");
		const auto fileName = req->getParameter("imageName");
		if (!isSafePathSegment(ownerId) || !isSafePathSegment(fileName)) {
			callback(jsonResponse(400, "Invalid image path", drogon::k400BadRequest));
			return;
		}

		auto fileDir = (kImageRoot / ownerId).string();
		auto filePathOpt = FileService::getFilePath(fileDir, fileName);
		if (!filePathOpt.has_value()) {
			callback(jsonResponse(404, "File not found", drogon::k404NotFound));
			return;
		}

		const std::filesystem::path filePath(filePathOpt.value());
		std::ifstream input(filePath, std::ios::binary);
		char header[16]{};
		input.read(header, sizeof(header));
		const auto mime = detectImageMime(std::string_view(header, static_cast<std::size_t>(input.gcount())));
		if (mime.empty()) {
			callback(jsonResponse(415, "Unsupported image format", drogon::k415UnsupportedMediaType));
			return;
		}

		const auto etag = makeEtag(filePath);
		if (req->getHeader("If-None-Match") == etag) {
			auto response = HttpResponse::newHttpResponse();
			response->setStatusCode(drogon::k304NotModified);
			response->addHeader("ETag", etag);
			response->addHeader("Cache-Control", req->getParameter("privacy") == "1"
				? "no-store, max-age=0" : "private, max-age=31536000, immutable");
			callback(response);
			return;
		}

		auto response = HttpResponse::newFileResponse(filePath.string());
		response->setContentTypeString(mime);
		response->addHeader("ETag", etag);
		response->addHeader("Cache-Control", req->getParameter("privacy") == "1"
			? "no-store, max-age=0" : "private, max-age=31536000, immutable");
		response->addHeader("X-Content-Type-Options", "nosniff");
		callback(response);
	}
	// - userName: 用户名
	// - imageName: 带扩展名的图片名
	// - file: 图片二进制内容
	void FilesController::upLoadImage(const HttpRequestPtr& req,
		std::function<void(const HttpResponsePtr&)>&& callback)
	{
		const auto ownerId = req->getParameter("userName");
		const auto requestedName = req->getParameter("imageName");
		if (!isSafePathSegment(ownerId) || !isSafePathSegment(requestedName)) {
			callback(jsonResponse(105, "Invalid image path", drogon::k400BadRequest));
			return;
		}

		auto contentType = req->getContentType();
		if (contentType == CT_MULTIPART_FORM_DATA) {
			MultiPartParser parser;
			int result = parser.parse(req);
			if (result != 0) {
				callback(jsonResponse(101, "Failed to parse multipart/form-data request", drogon::k400BadRequest));
				return;
			}

			const auto& files = parser.getFiles();
			if (files.size() != 1) {
				callback(jsonResponse(102, "Exactly one image is required", drogon::k400BadRequest));
				return;
			}

			const auto& uploadedFile = files[0];
			if (uploadedFile.fileLength() == 0 || uploadedFile.fileLength() > kMaxImageBytes) {
				callback(jsonResponse(106, "Image must be between 1 byte and 5 MB", drogon::k413RequestEntityTooLarge));
				return;
			}
			if (uploadedFile.getFileName() != requestedName) {
				callback(jsonResponse(107, "Image name mismatch", drogon::k400BadRequest));
				return;
			}

			const auto mime = detectImageMime(uploadedFile.fileContent());
			if (mime.empty()) {
				callback(jsonResponse(108, "Only JPEG, PNG and WebP images are supported", drogon::k415UnsupportedMediaType));
				return;
			}

			const auto ownerDir = kImageRoot / ownerId;
			std::error_code directoryError;
			std::filesystem::create_directories(ownerDir, directoryError);
			if (directoryError) {
				callback(jsonResponse(103, "Failed to create image directory", drogon::k500InternalServerError));
				return;
			}

			const auto destination = ownerDir / requestedName;
			const auto temporary = ownerDir / (requestedName + ".uploading");
			int ret = uploadedFile.saveAs(temporary.string());

			if (ret != 0) {
				callback(jsonResponse(103, "Failed to save image", drogon::k500InternalServerError));
				return;
			}

			std::error_code renameError;
			std::filesystem::rename(temporary, destination, renameError);
			if (renameError) {
				std::error_code removeError;
				std::filesystem::remove(temporary, removeError);
				callback(jsonResponse(103, "Failed to finalize image", drogon::k500InternalServerError));
				return;
			}
			schedulePrivacyUploadCleanup(req, destination);

			Json::Value json;
			json["code"] = 100;
			json["message"] = "File uploaded successfully";
			json["imageName"] = requestedName;
			json["mimeType"] = mime;
			json["byteSize"] = static_cast<Json::UInt64>(uploadedFile.fileLength());
			auto resp = HttpResponse::newHttpJsonResponse(json);
			resp->setStatusCode(drogon::k200OK);
			callback(resp);
			return;
		}
		callback(jsonResponse(104, "Unsupported Content-Type", drogon::k415UnsupportedMediaType));
	}

	void FilesController::loadVideo(const HttpRequestPtr& req,
		std::function<void(const HttpResponsePtr&)>&& callback)
	{
		const auto ownerId = req->getParameter("userName");
		const auto videoName = req->getParameter("videoName");
		if (!isSafePathSegment(ownerId) || !isSafePathSegment(videoName)) {
			callback(jsonResponse(400, "Invalid video path", drogon::k400BadRequest));
			return;
		}

		const auto filePath = FileService::getFilePath(
			(kVideoRoot / ownerId).string(), videoName);
		if (!filePath) {
			callback(jsonResponse(404, "Video not found", drogon::k404NotFound));
			return;
		}

		std::ifstream input(*filePath, std::ios::binary);
		char header[16]{};
		input.read(header, sizeof(header));
		const auto mime = detectVideoMime(
			videoName,
			std::string_view(header, static_cast<std::size_t>(input.gcount())));
		if (mime.empty()) {
			callback(jsonResponse(415, "Unsupported video format", drogon::k415UnsupportedMediaType));
			return;
		}

		// 传入原始请求后 Drogon 会用 sendfile 和 Range 响应按需传输，
		// 不会把整个视频加载到服务端内存。
		auto response = HttpResponse::newFileResponse(
			*filePath, "", drogon::CT_CUSTOM, mime, req);
		response->addHeader("Accept-Ranges", "bytes");
		response->addHeader("Cache-Control", req->getParameter("privacy") == "1"
			? "no-store, max-age=0" : "private, max-age=31536000, immutable");
		response->addHeader("X-Content-Type-Options", "nosniff");
		callback(response);
	}

	void FilesController::upLoadVideo(const HttpRequestPtr& req,
		std::function<void(const HttpResponsePtr&)>&& callback)
	{
		const auto ownerId = req->getParameter("userName");
		const auto requestedName = req->getParameter("videoName");
		if (!isSafePathSegment(ownerId) || !isSafePathSegment(requestedName)) {
			callback(jsonResponse(105, "Invalid video path", drogon::k400BadRequest));
			return;
		}
		if (req->getContentType() != CT_MULTIPART_FORM_DATA) {
			callback(jsonResponse(104, "Unsupported Content-Type", drogon::k415UnsupportedMediaType));
			return;
		}

		MultiPartParser parser;
		if (parser.parse(req) != 0) {
			callback(jsonResponse(101, "Failed to parse video upload", drogon::k400BadRequest));
			return;
		}
		const auto& files = parser.getFiles();
		if (files.size() != 1) {
			callback(jsonResponse(102, "Exactly one video is required", drogon::k400BadRequest));
			return;
		}

		const auto& video = files[0];
		if (video.fileLength() == 0 || video.fileLength() > kMaxVideoBytes) {
			callback(jsonResponse(106, "Video must be between 1 byte and 300 MB", drogon::k413RequestEntityTooLarge));
			return;
		}
		if (video.getFileName() != requestedName) {
			callback(jsonResponse(107, "Video name mismatch", drogon::k400BadRequest));
			return;
		}
		const auto mime = detectVideoMime(requestedName, video.fileContent());
		if (mime.empty()) {
			callback(jsonResponse(108, "Only MP4, M4V and MOV videos are supported", drogon::k415UnsupportedMediaType));
			return;
		}

		const auto ownerDir = kVideoRoot / ownerId;
		std::error_code directoryError;
		std::filesystem::create_directories(ownerDir, directoryError);
		if (directoryError) {
			callback(jsonResponse(103, "Failed to create video directory", drogon::k500InternalServerError));
			return;
		}

		const auto destination = ownerDir / requestedName;
		const auto temporary = ownerDir / (requestedName + ".uploading");
		if (video.saveAs(temporary.string()) != 0) {
			callback(jsonResponse(103, "Failed to save video", drogon::k500InternalServerError));
			return;
		}
		std::error_code renameError;
		std::filesystem::rename(temporary, destination, renameError);
		if (renameError) {
			std::error_code removeError;
			std::filesystem::remove(temporary, removeError);
			callback(jsonResponse(103, "Failed to finalize video", drogon::k500InternalServerError));
			return;
		}
		schedulePrivacyUploadCleanup(req, destination);

		Json::Value json;
		json["code"] = 100;
		json["message"] = "Video uploaded successfully";
		json["videoName"] = requestedName;
		json["mimeType"] = mime;
		json["byteSize"] = static_cast<Json::UInt64>(video.fileLength());
		auto response = HttpResponse::newHttpJsonResponse(json);
		response->setStatusCode(drogon::k200OK);
		callback(response);
	}

	void FilesController::loadAudio(const HttpRequestPtr& req,
		std::function<void(const HttpResponsePtr&)>&& callback)
	{
		const auto ownerId = req->getParameter("userName");
		const auto audioName = req->getParameter("audioName");
		if (!isSafePathSegment(ownerId) || !isSafePathSegment(audioName)) {
			callback(jsonResponse(400, "Invalid audio path", drogon::k400BadRequest));
			return;
		}
		const auto filePath = FileService::getFilePath(
			(kAudioRoot / ownerId).string(), audioName);
		if (!filePath) {
			callback(jsonResponse(404, "Audio not found", drogon::k404NotFound));
			return;
		}
		std::ifstream input(*filePath, std::ios::binary);
		char header[16]{};
		input.read(header, sizeof(header));
		const auto mime = detectAudioMime(
			audioName,
			std::string_view(header, static_cast<std::size_t>(input.gcount())));
		if (mime.empty()) {
			callback(jsonResponse(415, "Unsupported audio format", drogon::k415UnsupportedMediaType));
			return;
		}
		auto response = HttpResponse::newFileResponse(
			*filePath, "", drogon::CT_CUSTOM, mime, req);
		response->addHeader("Accept-Ranges", "bytes");
		response->addHeader("Cache-Control", req->getParameter("privacy") == "1"
			? "no-store, max-age=0" : "private, max-age=31536000, immutable");
		response->addHeader("X-Content-Type-Options", "nosniff");
		callback(response);
	}

	void FilesController::upLoadAudio(const HttpRequestPtr& req,
		std::function<void(const HttpResponsePtr&)>&& callback)
	{
		const auto ownerId = req->getParameter("userName");
		const auto requestedName = req->getParameter("audioName");
		if (!isSafePathSegment(ownerId) || !isSafePathSegment(requestedName)) {
			callback(jsonResponse(105, "Invalid audio path", drogon::k400BadRequest));
			return;
		}
		if (req->getContentType() != CT_MULTIPART_FORM_DATA) {
			callback(jsonResponse(104, "Unsupported Content-Type", drogon::k415UnsupportedMediaType));
			return;
		}
		MultiPartParser parser;
		if (parser.parse(req) != 0) {
			callback(jsonResponse(101, "Failed to parse audio upload", drogon::k400BadRequest));
			return;
		}
		const auto& files = parser.getFiles();
		if (files.size() != 1) {
			callback(jsonResponse(102, "Exactly one audio file is required", drogon::k400BadRequest));
			return;
		}
		const auto& audio = files[0];
		if (audio.fileLength() == 0 || audio.fileLength() > kMaxAudioBytes) {
			callback(jsonResponse(106, "Audio must be between 1 byte and 10 MB", drogon::k413RequestEntityTooLarge));
			return;
		}
		if (audio.getFileName() != requestedName) {
			callback(jsonResponse(107, "Audio name mismatch", drogon::k400BadRequest));
			return;
		}
		const auto mime = detectAudioMime(requestedName, audio.fileContent());
		if (mime.empty()) {
			callback(jsonResponse(108, "Only M4A/AAC audio is supported", drogon::k415UnsupportedMediaType));
			return;
		}
		const auto ownerDir = kAudioRoot / ownerId;
		std::error_code directoryError;
		std::filesystem::create_directories(ownerDir, directoryError);
		if (directoryError) {
			callback(jsonResponse(103, "Failed to create audio directory", drogon::k500InternalServerError));
			return;
		}
		const auto destination = ownerDir / requestedName;
		const auto temporary = ownerDir / (requestedName + ".uploading");
		if (audio.saveAs(temporary.string()) != 0) {
			callback(jsonResponse(103, "Failed to save audio", drogon::k500InternalServerError));
			return;
		}
		std::error_code renameError;
		std::filesystem::rename(temporary, destination, renameError);
		if (renameError) {
			std::error_code removeError;
			std::filesystem::remove(temporary, removeError);
			callback(jsonResponse(103, "Failed to finalize audio", drogon::k500InternalServerError));
			return;
		}
		schedulePrivacyUploadCleanup(req, destination);
		Json::Value json;
		json["code"] = 100;
		json["message"] = "Audio uploaded successfully";
		json["audioName"] = requestedName;
		json["mimeType"] = mime;
		json["byteSize"] = static_cast<Json::UInt64>(audio.fileLength());
		auto response = HttpResponse::newHttpJsonResponse(json);
		response->setStatusCode(drogon::k200OK);
		callback(response);
	}

	void FilesController::loadChatFile(const HttpRequestPtr& req,
		std::function<void(const HttpResponsePtr&)>&& callback)
	{
		const auto ownerId = req->getParameter("userName");
		const auto fileName = req->getParameter("fileName");
		if (!isSafePathSegment(ownerId) || !isSafePathSegment(fileName)) {
			callback(jsonResponse(400, "Invalid file path", drogon::k400BadRequest));
			return;
		}
		const auto filePath = FileService::getFilePath(
			(kChatFileRoot / ownerId).string(), fileName);
		if (!filePath) {
			callback(jsonResponse(404, "File not found", drogon::k404NotFound));
			return;
		}
		auto response = HttpResponse::newFileResponse(*filePath);
		response->setContentTypeCode(drogon::CT_APPLICATION_OCTET_STREAM);
		response->addHeader("Accept-Ranges", "bytes");
		response->addHeader("Cache-Control", req->getParameter("privacy") == "1"
			? "no-store, max-age=0" : "private, max-age=31536000, immutable");
		response->addHeader("X-Content-Type-Options", "nosniff");
		callback(response);
	}

	void FilesController::upLoadChatFile(const HttpRequestPtr& req,
		std::function<void(const HttpResponsePtr&)>&& callback)
	{
		const auto ownerId = req->getParameter("userName");
		const auto requestedName = req->getParameter("fileName");
		if (!isSafePathSegment(ownerId) || !isSafePathSegment(requestedName)) {
			callback(jsonResponse(105, "Invalid file path", drogon::k400BadRequest));
			return;
		}
		if (req->getContentType() != CT_MULTIPART_FORM_DATA) {
			callback(jsonResponse(104, "Unsupported Content-Type", drogon::k415UnsupportedMediaType));
			return;
		}
		MultiPartParser parser;
		if (parser.parse(req) != 0) {
			callback(jsonResponse(101, "Failed to parse file upload", drogon::k400BadRequest));
			return;
		}
		const auto& files = parser.getFiles();
		if (files.size() != 1) {
			callback(jsonResponse(102, "Exactly one file is required", drogon::k400BadRequest));
			return;
		}
		const auto& file = files[0];
		if (file.fileLength() == 0 || file.fileLength() > kMaxChatFileBytes) {
			callback(jsonResponse(106, "File must be between 1 byte and 300 MB", drogon::k413RequestEntityTooLarge));
			return;
		}
		if (file.getFileName() != requestedName) {
			callback(jsonResponse(107, "File name mismatch", drogon::k400BadRequest));
			return;
		}

		const auto ownerDir = kChatFileRoot / ownerId;
		std::error_code directoryError;
		std::filesystem::create_directories(ownerDir, directoryError);
		if (directoryError) {
			callback(jsonResponse(103, "Failed to create file directory", drogon::k500InternalServerError));
			return;
		}
		const auto destination = ownerDir / requestedName;
		const auto temporary = ownerDir / (requestedName + ".uploading");
		if (file.saveAs(temporary.string()) != 0) {
			callback(jsonResponse(103, "Failed to save file", drogon::k500InternalServerError));
			return;
		}
		std::error_code renameError;
		std::filesystem::rename(temporary, destination, renameError);
		if (renameError) {
			std::error_code removeError;
			std::filesystem::remove(temporary, removeError);
			callback(jsonResponse(103, "Failed to finalize file", drogon::k500InternalServerError));
			return;
		}
		schedulePrivacyUploadCleanup(req, destination);

		Json::Value json;
		json["code"] = 100;
		json["message"] = "File uploaded successfully";
		json["fileName"] = requestedName;
		json["byteSize"] = static_cast<Json::UInt64>(file.fileLength());
		auto response = HttpResponse::newHttpJsonResponse(json);
		response->setStatusCode(drogon::k200OK);
		callback(response);
	}
}
