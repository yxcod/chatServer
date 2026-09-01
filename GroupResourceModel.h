#pragma once

#include <cstdint>
#include <string>
#include <utility>

class GroupResourceModel
{
public:
    std::uint64_t getResourceId() const noexcept { return resourceId_; }
    std::uint64_t getGroupId() const noexcept { return groupId_; }
    std::uint8_t getResourceType() const noexcept { return resourceType_; }
    const std::string& getOriginalName() const noexcept { return originalName_; }
    const std::string& getStoredName() const noexcept { return storedName_; }
    const std::string& getCoverStoredName() const noexcept { return coverStoredName_; }
    const std::string& getMimeType() const noexcept { return mimeType_; }
    std::uint64_t getFileSize() const noexcept { return fileSize_; }
    const std::string& getUploaderId() const noexcept { return uploaderId_; }
    std::uint64_t getCreatedAt() const noexcept { return createdAt_; }

    void setResourceId(std::uint64_t value) noexcept { resourceId_ = value; }
    void setGroupId(std::uint64_t value) noexcept { groupId_ = value; }
    void setResourceType(std::uint8_t value) noexcept { resourceType_ = value; }
    void setOriginalName(std::string value) { originalName_ = std::move(value); }
    void setStoredName(std::string value) { storedName_ = std::move(value); }
    void setCoverStoredName(std::string value) { coverStoredName_ = std::move(value); }
    void setMimeType(std::string value) { mimeType_ = std::move(value); }
    void setFileSize(std::uint64_t value) noexcept { fileSize_ = value; }
    void setUploaderId(std::string value) { uploaderId_ = std::move(value); }
    void setCreatedAt(std::uint64_t value) noexcept { createdAt_ = value; }

private:
    std::uint64_t resourceId_{0};
    std::uint64_t groupId_{0};
    std::uint8_t resourceType_{0}; // 1-file, 2-album photo/video
    std::string originalName_;
    std::string storedName_;
    std::string coverStoredName_;
    std::string mimeType_;
    std::uint64_t fileSize_{0};
    std::string uploaderId_;
    std::uint64_t createdAt_{0};
};
