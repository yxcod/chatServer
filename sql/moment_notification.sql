-- 动态点赞和评论通知。请在部署本次后端代码前执行本文件。
CREATE TABLE IF NOT EXISTS `momentNotification` (
  `notificationId` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `recipientUserName` VARCHAR(50) NOT NULL,
  `actorUserName` VARCHAR(50) NOT NULL,
  `momentId` BIGINT UNSIGNED NOT NULL,
  `interactionType` TINYINT UNSIGNED NOT NULL COMMENT '1=点赞, 2=评论',
  `commentContent` VARCHAR(1000) NOT NULL DEFAULT '',
  `isRead` TINYINT(1) NOT NULL DEFAULT 0,
  `createdAt` BIGINT UNSIGNED NOT NULL,
  `readAt` BIGINT UNSIGNED NULL,
  PRIMARY KEY (`notificationId`),
  KEY `idx_moment_notification_recipient`
    (`recipientUserName`, `isRead`, `notificationId`),
  KEY `idx_moment_notification_moment` (`momentId`),
  KEY `idx_moment_notification_actor` (`actorUserName`, `createdAt`)
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  COLLATE=utf8mb4_0900_ai_ci;
