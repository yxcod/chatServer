CREATE TABLE IF NOT EXISTS `userLocation` (
  `userName` VARCHAR(64) NOT NULL,
  `latitude` DECIMAL(10,7) NOT NULL,
  `longitude` DECIMAL(10,7) NOT NULL,
  `accuracy` DECIMAL(10,2) NOT NULL DEFAULT 0,
  `updatedAt` BIGINT UNSIGNED NOT NULL,
  PRIMARY KEY (`userName`),
  KEY `idx_user_location_updated` (`updatedAt`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
