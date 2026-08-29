-- 商家点评系统。请在部署本次后端代码前执行本文件。
CREATE TABLE IF NOT EXISTS `merchantReviewEntry` (
  `entryId` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `ownerUserName` VARCHAR(50) NOT NULL,
  `poiId` VARCHAR(128) NOT NULL,
  `merchantName` VARCHAR(160) NOT NULL,
  `address` VARCHAR(500) NOT NULL DEFAULT '',
  `category` VARCHAR(200) NOT NULL DEFAULT '',
  `distanceMeters` INT UNSIGNED NULL,
  `rating` DECIMAL(4,2) NULL,
  `imageUrl` VARCHAR(2048) NOT NULL DEFAULT '',
  `imageUrlsJson` LONGTEXT NULL,
  `uploadedImagesJson` LONGTEXT NULL,
  `phone` VARCHAR(100) NOT NULL DEFAULT '',
  `openingHours` VARCHAR(500) NOT NULL DEFAULT '',
  `price` DECIMAL(10,2) NULL,
  `detailUrl` VARCHAR(2048) NOT NULL DEFAULT '',
  `imageCount` INT UNSIGNED NOT NULL DEFAULT 0,
  `latitude` DECIMAL(10,7) NULL,
  `longitude` DECIMAL(10,7) NULL,
  `likeCount` INT UNSIGNED NOT NULL DEFAULT 0,
  `dislikeCount` INT UNSIGNED NOT NULL DEFAULT 0,
  `commentCount` INT UNSIGNED NOT NULL DEFAULT 0,
  `status` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `createdAt` BIGINT UNSIGNED NOT NULL,
  `updatedAt` BIGINT UNSIGNED NOT NULL,
  PRIMARY KEY (`entryId`),
  UNIQUE KEY `uk_merchant_review_owner_poi` (`ownerUserName`, `poiId`),
  KEY `idx_merchant_review_owner_rank`
    (`ownerUserName`, `status`, `likeCount`, `dislikeCount`, `commentCount`),
  KEY `idx_merchant_review_poi` (`poiId`)
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `merchantReviewReaction` (
  `reactionId` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `entryId` BIGINT UNSIGNED NOT NULL,
  `userName` VARCHAR(50) NOT NULL,
  `reactionType` TINYINT UNSIGNED NOT NULL COMMENT '1=赞, 2=踩',
  `createdAt` BIGINT UNSIGNED NOT NULL,
  `updatedAt` BIGINT UNSIGNED NOT NULL,
  PRIMARY KEY (`reactionId`),
  UNIQUE KEY `uk_merchant_review_reaction_user` (`entryId`, `userName`),
  KEY `idx_merchant_review_reaction_user` (`userName`, `updatedAt`),
  CONSTRAINT `fk_merchant_review_reaction_entry`
    FOREIGN KEY (`entryId`) REFERENCES `merchantReviewEntry` (`entryId`)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `merchantReviewComment` (
  `commentId` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `entryId` BIGINT UNSIGNED NOT NULL,
  `userName` VARCHAR(50) NOT NULL,
  `content` VARCHAR(1000) NOT NULL,
  `imageName` VARCHAR(255) NOT NULL DEFAULT '',
  `status` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `createdAt` BIGINT UNSIGNED NOT NULL,
  `updatedAt` BIGINT UNSIGNED NOT NULL,
  `deletedAt` BIGINT UNSIGNED NULL,
  PRIMARY KEY (`commentId`),
  KEY `idx_merchant_review_comment_entry`
    (`entryId`, `status`, `createdAt`),
  KEY `idx_merchant_review_comment_user` (`userName`, `createdAt`),
  CONSTRAINT `fk_merchant_review_comment_entry`
    FOREIGN KEY (`entryId`) REFERENCES `merchantReviewEntry` (`entryId`)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  COLLATE=utf8mb4_unicode_ci;
