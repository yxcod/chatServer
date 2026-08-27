-- 用户空间背景墙与留言板。请在部署本次后端代码前执行本文件。
CREATE TABLE IF NOT EXISTS `userSpace` (
  `ownerUserName` VARCHAR(50) NOT NULL,
  `coverImageUrl` VARCHAR(2048) NOT NULL DEFAULT '',
  `createdAt` BIGINT UNSIGNED NOT NULL,
  `updatedAt` BIGINT UNSIGNED NOT NULL,
  PRIMARY KEY (`ownerUserName`)
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `spaceGuestbookMessage` (
  `messageId` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `ownerUserName` VARCHAR(50) NOT NULL,
  `authorUserName` VARCHAR(50) NOT NULL,
  `content` VARCHAR(500) NOT NULL,
  `status` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `createdAt` BIGINT UNSIGNED NOT NULL,
  `updatedAt` BIGINT UNSIGNED NOT NULL,
  `deletedAt` BIGINT UNSIGNED NULL,
  PRIMARY KEY (`messageId`),
  KEY `idx_space_guestbook_owner` (`ownerUserName`, `status`, `createdAt`),
  KEY `idx_space_guestbook_author` (`authorUserName`, `status`, `createdAt`)
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  COLLATE=utf8mb4_unicode_ci;
