CREATE TABLE private_chat_history_visibility (
    conversationId VARCHAR(64) NOT NULL,
    userId VARCHAR(64) NOT NULL,
    peerUserId VARCHAR(64) NOT NULL,
    deletedThroughRecordId BIGINT UNSIGNED NOT NULL DEFAULT 0,
    updatedAt BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (conversationId, userId),
    KEY idx_private_chat_visibility_user (userId, updatedAt)
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  COLLATE=utf8mb4_unicode_ci;
