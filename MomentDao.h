#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <jdbc/cppconn/connection.h>

#include "MomentCommentModel.h"
#include "MomentMediaModel.h"
#include "MomentModel.h"

class MomentDao
{
public:
    MomentModel createMoment(MomentModel moment,
                             const std::vector<MomentMediaModel>& media) const;
    std::vector<MomentModel> getOwnMoments(const std::string& userName,
                                           std::uint64_t beforeMomentId,
                                           unsigned int limit) const;
    std::vector<MomentModel> getVisibleMoments(const std::string& viewerUserName,
                                               const std::string& authorUserName,
                                               std::uint64_t beforeMomentId,
                                               unsigned int limit) const;
    MomentModel toggleLike(std::uint64_t momentId,
                           const std::string& userName,
                           std::uint64_t now) const;
    MomentModel addComment(std::uint64_t momentId,
                           const std::string& userName,
                           const std::string& content,
                           std::uint64_t now) const;
    std::vector<std::string> deleteMoment(std::uint64_t momentId,
                                          const std::string& authorUserName) const;
    std::string getAuthorUserName(std::uint64_t momentId) const;

private:
    MomentModel getMoment(sql::Connection* connection,
                          std::uint64_t momentId,
                          const std::string& viewerUserName) const;
    std::vector<MomentMediaModel> getMedia(sql::Connection* connection,
                                           std::uint64_t momentId) const;
    std::vector<MomentCommentModel> getComments(sql::Connection* connection,
                                                std::uint64_t momentId) const;
    void lockVisibleMoment(sql::Connection* connection,
                           std::uint64_t momentId,
                           const std::string& viewerUserName) const;
};
