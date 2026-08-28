CREATE TABLE IF NOT EXISTS `pushDeviceRegistration` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `userName` VARCHAR(64) NOT NULL,
  `registrationId` VARCHAR(255) NOT NULL,
  `platform` VARCHAR(16) NOT NULL DEFAULT 'android',
  `deviceId` VARCHAR(128) NOT NULL DEFAULT '',
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `appForeground` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `bannerEnabled` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `soundEnabled` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `vibrationEnabled` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `createdAt` BIGINT UNSIGNED NOT NULL,
  `updatedAt` BIGINT UNSIGNED NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_push_registration_id` (`registrationId`),
  KEY `idx_push_user_enabled_foreground` (`userName`, `enabled`, `appForeground`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
