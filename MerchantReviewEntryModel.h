#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "MerchantReviewCommentModel.h"

class MerchantReviewEntryModel
{
public:
    std::uint64_t getEntryId() const noexcept { return entryId_; }
    const std::string& getOwnerUserName() const noexcept { return ownerUserName_; }
    const std::string& getPoiId() const noexcept { return poiId_; }
    const std::string& getMerchantName() const noexcept { return merchantName_; }
    const std::string& getAddress() const noexcept { return address_; }
    const std::string& getCategory() const noexcept { return category_; }
    std::uint32_t getDistanceMeters() const noexcept { return distanceMeters_; }
    bool hasDistanceMeters() const noexcept { return hasDistanceMeters_; }
    double getRating() const noexcept { return rating_; }
    bool hasRating() const noexcept { return hasRating_; }
    const std::string& getImageUrl() const noexcept { return imageUrl_; }
    const std::string& getImageUrlsJson() const noexcept { return imageUrlsJson_; }
    const std::string& getUploadedImagesJson() const noexcept { return uploadedImagesJson_; }
    const std::string& getPhone() const noexcept { return phone_; }
    const std::string& getOpeningHours() const noexcept { return openingHours_; }
    double getPrice() const noexcept { return price_; }
    bool hasPrice() const noexcept { return hasPrice_; }
    const std::string& getDetailUrl() const noexcept { return detailUrl_; }
    std::uint32_t getImageCount() const noexcept { return imageCount_; }
    double getLatitude() const noexcept { return latitude_; }
    bool hasLatitude() const noexcept { return hasLatitude_; }
    double getLongitude() const noexcept { return longitude_; }
    bool hasLongitude() const noexcept { return hasLongitude_; }
    std::uint32_t getLikeCount() const noexcept { return likeCount_; }
    std::uint32_t getDislikeCount() const noexcept { return dislikeCount_; }
    std::uint32_t getCommentCount() const noexcept { return commentCount_; }
    std::uint8_t getStatus() const noexcept { return status_; }
    std::uint64_t getCreatedAt() const noexcept { return createdAt_; }
    std::uint64_t getUpdatedAt() const noexcept { return updatedAt_; }
    std::uint8_t getViewerReaction() const noexcept { return viewerReaction_; }
    const std::vector<MerchantReviewCommentModel>& getComments() const noexcept { return comments_; }

    void setEntryId(std::uint64_t value) noexcept { entryId_ = value; }
    void setOwnerUserName(std::string value) { ownerUserName_ = std::move(value); }
    void setPoiId(std::string value) { poiId_ = std::move(value); }
    void setMerchantName(std::string value) { merchantName_ = std::move(value); }
    void setAddress(std::string value) { address_ = std::move(value); }
    void setCategory(std::string value) { category_ = std::move(value); }
    void setDistanceMeters(std::uint32_t value) noexcept { distanceMeters_ = value; hasDistanceMeters_ = true; }
    void clearDistanceMeters() noexcept { distanceMeters_ = 0; hasDistanceMeters_ = false; }
    void setRating(double value) noexcept { rating_ = value; hasRating_ = true; }
    void clearRating() noexcept { rating_ = 0; hasRating_ = false; }
    void setImageUrl(std::string value) { imageUrl_ = std::move(value); }
    void setImageUrlsJson(std::string value) { imageUrlsJson_ = std::move(value); }
    void setUploadedImagesJson(std::string value) { uploadedImagesJson_ = std::move(value); }
    void setPhone(std::string value) { phone_ = std::move(value); }
    void setOpeningHours(std::string value) { openingHours_ = std::move(value); }
    void setPrice(double value) noexcept { price_ = value; hasPrice_ = true; }
    void clearPrice() noexcept { price_ = 0; hasPrice_ = false; }
    void setDetailUrl(std::string value) { detailUrl_ = std::move(value); }
    void setImageCount(std::uint32_t value) noexcept { imageCount_ = value; }
    void setLatitude(double value) noexcept { latitude_ = value; hasLatitude_ = true; }
    void clearLatitude() noexcept { latitude_ = 0; hasLatitude_ = false; }
    void setLongitude(double value) noexcept { longitude_ = value; hasLongitude_ = true; }
    void clearLongitude() noexcept { longitude_ = 0; hasLongitude_ = false; }
    void setLikeCount(std::uint32_t value) noexcept { likeCount_ = value; }
    void setDislikeCount(std::uint32_t value) noexcept { dislikeCount_ = value; }
    void setCommentCount(std::uint32_t value) noexcept { commentCount_ = value; }
    void setStatus(std::uint8_t value) noexcept { status_ = value; }
    void setCreatedAt(std::uint64_t value) noexcept { createdAt_ = value; }
    void setUpdatedAt(std::uint64_t value) noexcept { updatedAt_ = value; }
    void setViewerReaction(std::uint8_t value) noexcept { viewerReaction_ = value; }
    void setComments(std::vector<MerchantReviewCommentModel> value) { comments_ = std::move(value); }

private:
    std::uint64_t entryId_{0};
    std::string ownerUserName_;
    std::string poiId_;
    std::string merchantName_;
    std::string address_;
    std::string category_;
    std::uint32_t distanceMeters_{0};
    bool hasDistanceMeters_{false};
    double rating_{0};
    bool hasRating_{false};
    std::string imageUrl_;
    std::string imageUrlsJson_;
    std::string uploadedImagesJson_;
    std::string phone_;
    std::string openingHours_;
    double price_{0};
    bool hasPrice_{false};
    std::string detailUrl_;
    std::uint32_t imageCount_{0};
    double latitude_{0};
    bool hasLatitude_{false};
    double longitude_{0};
    bool hasLongitude_{false};
    std::uint32_t likeCount_{0};
    std::uint32_t dislikeCount_{0};
    std::uint32_t commentCount_{0};
    std::uint8_t status_{0};
    std::uint64_t createdAt_{0};
    std::uint64_t updatedAt_{0};
    std::uint8_t viewerReaction_{0};
    std::vector<MerchantReviewCommentModel> comments_;
};
