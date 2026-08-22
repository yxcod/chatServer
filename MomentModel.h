#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "MomentCommentModel.h"
#include "MomentMediaModel.h"

class MomentModel
{
public:
    std::uint64_t getMomentId() const noexcept { return momentId_; }
    const std::string& getAuthorUserName() const noexcept { return authorUserName_; }
    const std::string& getContent() const noexcept { return content_; }
    std::uint8_t getVisibility() const noexcept { return visibility_; }
    const std::string& getLocationName() const noexcept { return locationName_; }
    double getLatitude() const noexcept { return latitude_; }
    double getLongitude() const noexcept { return longitude_; }
    std::uint32_t getLikeCount() const noexcept { return likeCount_; }
    std::uint32_t getCommentCount() const noexcept { return commentCount_; }
    const std::string& getClientRequestId() const noexcept { return clientRequestId_; }
    std::uint8_t getStatus() const noexcept { return status_; }
    std::uint64_t getCreatedAt() const noexcept { return createdAt_; }
    std::uint64_t getUpdatedAt() const noexcept { return updatedAt_; }
    std::uint64_t getDeletedAt() const noexcept { return deletedAt_; }

    const std::string& getAuthorNickName() const noexcept { return authorNickName_; }
    const std::string& getAuthorAvatar() const noexcept { return authorAvatar_; }
    bool isLikedByViewer() const noexcept { return likedByViewer_; }
    const std::vector<MomentMediaModel>& getMedia() const noexcept { return media_; }
    const std::vector<MomentCommentModel>& getComments() const noexcept { return comments_; }

    void setMomentId(std::uint64_t value) noexcept { momentId_ = value; }
    void setAuthorUserName(std::string value) { authorUserName_ = std::move(value); }
    void setContent(std::string value) { content_ = std::move(value); }
    void setVisibility(std::uint8_t value) noexcept { visibility_ = value; }
    void setLocationName(std::string value) { locationName_ = std::move(value); }
    void setLatitude(double value) noexcept { latitude_ = value; }
    void setLongitude(double value) noexcept { longitude_ = value; }
    void setLikeCount(std::uint32_t value) noexcept { likeCount_ = value; }
    void setCommentCount(std::uint32_t value) noexcept { commentCount_ = value; }
    void setClientRequestId(std::string value) { clientRequestId_ = std::move(value); }
    void setStatus(std::uint8_t value) noexcept { status_ = value; }
    void setCreatedAt(std::uint64_t value) noexcept { createdAt_ = value; }
    void setUpdatedAt(std::uint64_t value) noexcept { updatedAt_ = value; }
    void setDeletedAt(std::uint64_t value) noexcept { deletedAt_ = value; }

    void setAuthorNickName(std::string value) { authorNickName_ = std::move(value); }
    void setAuthorAvatar(std::string value) { authorAvatar_ = std::move(value); }
    void setLikedByViewer(bool value) noexcept { likedByViewer_ = value; }
    void setMedia(std::vector<MomentMediaModel> value) { media_ = std::move(value); }
    void setComments(std::vector<MomentCommentModel> value) { comments_ = std::move(value); }

private:
    std::uint64_t momentId_{0};
    std::string authorUserName_;
    std::string content_;
    std::uint8_t visibility_{0};
    std::string locationName_;
    double latitude_{0};
    double longitude_{0};
    std::uint32_t likeCount_{0};
    std::uint32_t commentCount_{0};
    std::string clientRequestId_;
    std::uint8_t status_{0};
    std::uint64_t createdAt_{0};
    std::uint64_t updatedAt_{0};
    std::uint64_t deletedAt_{0};

    // Read-only projections joined from userinfo and viewer state.
    std::string authorNickName_;
    std::string authorAvatar_;
    bool likedByViewer_{false};
    std::vector<MomentMediaModel> media_;
    std::vector<MomentCommentModel> comments_;
};
