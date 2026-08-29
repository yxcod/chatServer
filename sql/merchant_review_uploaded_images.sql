-- Run once before deploying merchant review gallery support.
ALTER TABLE `merchantReviewEntry`
  ADD COLUMN `uploadedImagesJson` LONGTEXT NULL AFTER `imageUrlsJson`;
