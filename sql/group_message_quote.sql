ALTER TABLE `groupMessage`
  ADD COLUMN `extendInfo` VARCHAR(2048) NOT NULL DEFAULT '{}'
  AFTER `msgContent`;
