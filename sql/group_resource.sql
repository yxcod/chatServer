CREATE TABLE IF NOT EXISTS `groupResource` (
  `resourceId` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '群资源ID',
  `groupId` BIGINT UNSIGNED NOT NULL COMMENT '群ID',
  `resourceType` TINYINT UNSIGNED NOT NULL COMMENT '1群文件/视频，2群相册照片',
  `originalName` VARCHAR(255) NOT NULL COMMENT '用户上传时的原始文件名',
  `storedName` VARCHAR(255) NOT NULL COMMENT '服务器实际存储文件名',
  `coverStoredName` VARCHAR(255) NULL COMMENT '视频封面的服务器存储文件名',
  `mimeType` VARCHAR(128) NOT NULL DEFAULT 'application/octet-stream',
  `fileSize` BIGINT UNSIGNED NOT NULL DEFAULT 0,
  `uploaderId` VARCHAR(64) NOT NULL COMMENT '上传者账户',
  `createdAt` BIGINT UNSIGNED NOT NULL COMMENT '毫秒时间戳',
  `isDeleted` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `deletedBy` VARCHAR(64) NULL,
  `deletedAt` BIGINT UNSIGNED NULL,
  PRIMARY KEY (`resourceId`),
  UNIQUE KEY `uk_group_resource_stored` (`groupId`, `storedName`),
  KEY `idx_group_resource_list` (`groupId`, `resourceType`, `isDeleted`, `createdAt`),
  KEY `idx_group_resource_uploader` (`uploaderId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
