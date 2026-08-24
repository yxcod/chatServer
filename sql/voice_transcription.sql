CREATE TABLE IF NOT EXISTS `voiceTranscription` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `audioOwnerId` VARCHAR(64) NOT NULL,
  `audioName` VARCHAR(191) NOT NULL,
  `audioSha256` CHAR(64) NOT NULL,
  `engineType` VARCHAR(32) NOT NULL,
  `transcript` MEDIUMTEXT NOT NULL,
  `audioDurationMs` INT UNSIGNED NOT NULL DEFAULT 0,
  `providerRequestId` VARCHAR(64) NOT NULL DEFAULT '',
  `createdAt` BIGINT UNSIGNED NOT NULL,
  `updatedAt` BIGINT UNSIGNED NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_voice_transcription_audio`
    (`audioOwnerId`, `audioName`, `audioSha256`, `engineType`),
  KEY `idx_voice_transcription_updated` (`updatedAt`)
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  COLLATE=utf8mb4_unicode_ci;
