#include "FileService.h"
#include "RangeUtils.h"
#include <fstream>
#include <filesystem>
#include <drogon/drogon.h>

namespace fs = std::filesystem;
namespace api {

	bool FileService::uploadFile(const std::string& rootDir,
		const std::string& filename,
		const std::string& fileData) {
		try {
			std::string saveDir = drogon::app().getUploadPath() + "/" + rootDir;
			fs::create_directories(saveDir);
			std::string filePath = saveDir + "/" + filename;

			std::ofstream out(filePath, std::ios::binary);
			if (!out) return false;

			out.write(fileData.data(), static_cast<std::streamsize>(fileData.size()));
			return !!out;
		}
		catch (...) {
			return false;
		}
	}

	std::optional<std::string> FileService::getFilePath(const std::string& rootDir,
			const std::string& filename) {
		try {
			auto rootPath = fs::weakly_canonical(fs::path(rootDir));
			auto filePath = fs::weakly_canonical(rootPath / filename);

			auto rootIt = rootPath.begin();
			auto fileIt = filePath.begin();
			for (; rootIt != rootPath.end() && fileIt != filePath.end(); ++rootIt, ++fileIt) {
				if (*rootIt != *fileIt) {
					return std::nullopt;
				}
			}
			if (rootIt != rootPath.end() || !fs::exists(filePath) || !fs::is_regular_file(filePath)) {
				return std::nullopt;
			}

			return filePath.string();
		}
		catch (...) {
			return std::nullopt;
		}
	}

	std::optional<std::pair<size_t, size_t>> FileService::parseRangeHeader(
		const std::string& rangeHeader,
		size_t fileSize) {

		std::vector<std::pair<size_t, size_t>> ranges;
		if (!RangeUtils::parseByteRanges(rangeHeader, fileSize, ranges) || ranges.empty()) {
			return std::nullopt;
		}

		return ranges[0]; // 只取第一个 RANGE 段
	}

	std::optional<std::pair<std::string, size_t>> FileService::readFileRange(
		const std::string& rootDir,
		const std::string& filename,
		size_t start,
		size_t end) {

		auto filePathOpt = getFilePath(rootDir, filename);
		if (!filePathOpt.has_value()) {
			return std::nullopt;
		}

		auto filePath = filePathOpt.value();
		auto fileSize = fs::file_size(filePath);

		if (start >= fileSize) {
			return std::nullopt;
		}

		if (end >= fileSize) {
			end = fileSize - 1;
		}

		std::string data = RangeUtils::readFileToMemory(filePath, start, end - start + 1);

		return std::make_pair(std::move(data), fileSize);
	}

}  // namespace api
