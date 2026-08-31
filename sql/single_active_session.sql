-- Execute once before deploying the matching backend build.
-- Every successful login invalidates all older tokens for that account.
ALTER TABLE login
    ADD COLUMN sessionVersion BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER salt;
