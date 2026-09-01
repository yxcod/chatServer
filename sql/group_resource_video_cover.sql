-- 已存在 groupResource 表的数据库执行一次本迁移。
-- 旧数据保持 NULL，客户端会继续使用原有视频首帧回退逻辑。
ALTER TABLE `groupResource`
  ADD COLUMN `coverStoredName` VARCHAR(255) NULL
  COMMENT '视频封面的服务器存储文件名'
  AFTER `storedName`;
