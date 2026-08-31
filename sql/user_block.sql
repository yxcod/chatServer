CREATE TABLE IF NOT EXISTS `userBlock` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `blockerUserName` VARCHAR(50) NOT NULL,
  `blockedUserName` VARCHAR(50) NOT NULL,
  `createdAt` BIGINT UNSIGNED NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_user_block_pair` (`blockerUserName`, `blockedUserName`),
  KEY `idx_user_block_blocked` (`blockedUserName`),
  KEY `idx_user_block_created` (`blockerUserName`, `createdAt`)
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  COLLATE=utf8mb4_0900_ai_ci;
