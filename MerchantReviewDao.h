#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <jdbc/cppconn/connection.h>

#include "MerchantReviewEntryModel.h"

class MerchantReviewDao
{
public:
    MerchantReviewEntryModel addEntry(MerchantReviewEntryModel entry) const;
    std::vector<MerchantReviewEntryModel> listEntries(
        const std::string& ownerUserName,
        const std::string& viewerUserName,
        unsigned int limit) const;
    MerchantReviewEntryModel setReaction(std::uint64_t entryId,
                                         const std::string& userName,
                                         std::uint8_t reactionType,
                                         std::uint64_t now) const;
    MerchantReviewEntryModel addComment(std::uint64_t entryId,
                                        const std::string& userName,
                                        const std::string& content,
                                        const std::string& imageName,
                                        std::uint64_t now) const;
    MerchantReviewEntryModel removeComment(std::uint64_t entryId,
                                           std::uint64_t commentId,
                                           const std::string& userName,
                                           std::uint64_t now) const;
    MerchantReviewEntryModel setUploadedImages(
        std::uint64_t entryId,
        const std::string& ownerUserName,
        const std::string& uploadedImagesJson,
        std::uint64_t now) const;
    void removeEntry(std::uint64_t entryId,
                     const std::string& ownerUserName) const;

private:
    MerchantReviewEntryModel getEntry(sql::Connection* connection,
                                      std::uint64_t entryId,
                                      const std::string& viewerUserName) const;
    std::vector<MerchantReviewCommentModel> getComments(
        sql::Connection* connection,
        std::uint64_t entryId) const;
    void lockEntry(sql::Connection* connection, std::uint64_t entryId) const;
};
