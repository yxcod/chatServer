#pragma once

#include <cstdint>
#include <string>
#include <utility>

class MomentMediaModel
{
public:
    std::uint64_t getMediaId() const noexcept { return mediaId_; }
    std::uint64_t getMomentId() const noexcept { return momentId_; }
    std::uint8_t getMediaType() const noexcept { return mediaType_; }
    const std::string& getMediaUrl() const noexcept { return mediaUrl_; }
    const std::string& getThumbnailUrl() const noexcept { return thumbnailUrl_; }
    std::uint32_t getWidth() const noexcept { return width_; }
    std::uint32_t getHeight() const noexcept { return height_; }
    std::uint64_t getFileSize() const noexcept { return fileSize_; }
    const std::string& getFileHash() const noexcept { return fileHash_; }
    std::uint16_t getSortOrder() const noexcept { return sortOrder_; }
    std::uint64_t getCreatedAt() const noexcept { return createdAt_; }

    void setMediaId(std::uint64_t value) noexcept { mediaId_ = value; }
    void setMomentId(std::uint64_t value) noexcept { momentId_ = value; }
    void setMediaType(std::uint8_t value) noexcept { mediaType_ = value; }
    void setMediaUrl(std::string value) { mediaUrl_ = std::move(value); }
    void setThumbnailUrl(std::string value) { thumbnailUrl_ = std::move(value); }
    void setWidth(std::uint32_t value) noexcept { width_ = value; }
    void setHeight(std::uint32_t value) noexcept { height_ = value; }
    void setFileSize(std::uint64_t value) noexcept { fileSize_ = value; }
    void setFileHash(std::string value) { fileHash_ = std::move(value); }
    void setSortOrder(std::uint16_t value) noexcept { sortOrder_ = value; }
    void setCreatedAt(std::uint64_t value) noexcept { createdAt_ = value; }

private:
    std::uint64_t mediaId_{0};
    std::uint64_t momentId_{0};
    std::uint8_t mediaType_{0};
    std::string mediaUrl_;
    std::string thumbnailUrl_;
    std::uint32_t width_{0};
    std::uint32_t height_{0};
    std::uint64_t fileSize_{0};
    std::string fileHash_;
    std::uint16_t sortOrder_{0};
    std::uint64_t createdAt_{0};
};
