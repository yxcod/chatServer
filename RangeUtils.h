#pragma once
#include <fstream>
#include <sstream>
#include <vector>
class RangeUtils {
public:
    // 解析 Range 头部，如 "bytes=0-9,20-30"
    static bool parseByteRanges(const std::string& rangeHeader,
        size_t fileSize,
        std::vector<std::pair<size_t, size_t>>& ranges);

    // 从文件中读取指定偏移和长度的内容
    static std::string readFileToMemory(const std::string& filePath,
        size_t offset,
        size_t length);
};


inline bool RangeUtils::parseByteRanges(const std::string& rangeHeader,
    size_t fileSize,
    std::vector<std::pair<size_t, size_t>>& ranges) {
    ranges.clear();

    const std::string prefix = "bytes=";
    if (rangeHeader.compare(0, prefix.size(), prefix) != 0) {
        return false;// 不是 bytes 范围请求
    }

    std::string rangesStr = rangeHeader.substr(prefix.size());
    std::istringstream ss(rangesStr);
    std::string range;

    while (std::getline(ss, range, ',')) {
        size_t dashPos = range.find('-');
        if (dashPos == std::string::npos) continue;

        std::string startStr = range.substr(0, dashPos);
        std::string endStr = range.substr(dashPos + 1);

        // 去除前后空格
        startStr.erase(0, startStr.find_first_not_of(" \t"));
        startStr.erase(startStr.find_last_not_of(" \t") + 1);
        endStr.erase(0, endStr.find_first_not_of(" \t"));
        endStr.erase(endStr.find_last_not_of(" \t") + 1);

        if (startStr.empty() || endStr.empty()) continue;

        try {
            size_t start = std::stoull(startStr);
            size_t end = std::stoull(endStr);

            if (end >= fileSize) end = fileSize - 1;
            if (start > end) continue;

            ranges.emplace_back(start, end);
        }
        catch (...) {
            return false;
        }
    }

    return !ranges.empty();
}

inline std::string RangeUtils::readFileToMemory(const std::string& filePath,
    size_t offset,
    size_t length) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) return "";

    file.seekg(offset, std::ios::beg);
    std::string buffer(length, '\0');
    file.read(&buffer[0], length);
    buffer.resize(file.gcount());// 实际读了多少字节
    return buffer;
}
