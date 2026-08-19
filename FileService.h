#pragma once
#include <string>
#include <optional>
#include <utility>

namespace api {

	class FileService {
	public:
		// 上传文件（支持 raw data 或 multipart/form-data）
		static bool uploadFile(const std::string& rootDir,
			const std::string& filename,
			const std::string& fileData);

		// 下载文件（返回完整的文件路径）
		static std::optional<std::string> getFilePath(const std::string& rootDir,
			const std::string& filename);

		// 解析 RANGE 请求头并返回第一个有效范围
		static std::optional<std::pair<size_t, size_t>> parseRangeHeader(
			const std::string& rangeHeader,
			size_t fileSize);

		// 读取指定范围的文件内容（用于流式传输）
		static std::optional<std::pair<std::string, size_t>> readFileRange(
			const std::string& rootDir,
			const std::string& filename,
			size_t start,
			size_t end);
	};

}  // namespace api
