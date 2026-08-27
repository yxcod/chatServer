-- Align existing user-space tables with the MySQL 8 userinfo collation.
-- Back up first. This conversion preserves current covers and messages.
ALTER TABLE `userSpace`
  CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci;

ALTER TABLE `spaceGuestbookMessage`
  CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci;
