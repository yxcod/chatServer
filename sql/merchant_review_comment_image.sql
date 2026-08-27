-- Run once before deploying image comments.
ALTER TABLE `merchantReviewComment`
  ADD COLUMN `imageName` VARCHAR(255) NOT NULL DEFAULT '' AFTER `content`;
