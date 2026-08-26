# Changelog

## 2026-08-25

- Added authenticated owner-only moment deletion, including transactional
  cleanup of likes, comments, media rows, and best-effort media file removal.
- Added dedicated chat-file upload and download endpoints with a strict
  300 MB per-file limit and owner-scoped storage.
- Added private/group conversation previews for voice, video, and file
  messages.
- Fixed Tencent Flash ASR requests so Drogon no longer percent-encodes the
  signed query separator, which previously produced non-JSON gateway replies.
- Added content-type-independent Tencent response parsing and safe HTTP/body
  diagnostics for failed transcription requests.
- Removed a duplicate manual Content-Length header; Drogon already emits the
  binary body length and nginx rejects duplicate values with HTTP 400.

## 2026-08-24 - Persistent quoted messages

- Added `extendInfo` persistence for group messages so quoted-message metadata survives history reloads.
- Included private and group message extension data in history API responses.

## 2026-08-24 - Tencent Cloud voice transcription

- Added authenticated `/api/audio/transcribe` support for M4A/AAC chat audio.
- Added Tencent Cloud Flash ASR signing and HTTPS integration using a Git-ignored local config.
- Added the `voiceTranscription` model, DAO, SQL schema, and content-hash cache to avoid repeated recognition calls.

All notable project changes are documented in this file.

## [Unreleased]

### Changed

- JWT signing now uses a fixed in-code secret, so local startup and login no
  longer require the `CHATSERVER_JWT_SECRET` environment variable.
- Documented that production deployments should move the JWT secret back to a
  secure external configuration before public release.

### Documentation

- Added Chinese usage comments for every `PooledConnection` member function,
  including ownership transfer, connection access, and automatic pool return.
- Established the convention that newly added or modified functions require
  purpose comments, with additional explanation for complex logic.

### Fixed

- Replaced Chinese relationship-status literals with UTF-8 universal escapes,
  preventing code page 936 from corrupting string boundaries during MSVC builds.
- Enabled UTF-8 compilation for `FriendRelationModel.cpp` so Chinese function
  comments cannot corrupt the following declaration under the legacy code page.
- Fixed privacy-message delay compilation on Windows by preventing `min` and
  `max` macros from expanding the qualified standard-library calls.
- Enabled UTF-8 compilation for `ChatManageController.cpp` so its Chinese
  comments are parsed consistently instead of using Windows code page 936.
- Fixed Windows builds of Tencent ASR by loading Winsock2 before `Windows.h`,
  preventing conflicts with Drogon's `ws2tcpip.h` declarations.
- Enabled UTF-8 compilation for `UserModel.cpp`, preventing Chinese string
  literals such as gender labels from being misparsed under code page 936.
- Fixed the MSVC build failure in private chat history deletion caused by the
  Windows `min` macro expanding the qualified `std::min` call.
- Enabled UTF-8 compilation for `ChatDao.cpp` so Chinese comments and existing
  Unicode text no longer corrupt function parsing under Windows code page 936.
- Added the MSVC `/FS` compiler option to every build configuration, preventing
  parallel compiler processes from failing with C1041 while writing `vc143.pdb`.

## [2026-08-19]

### Added

- Added a thread-safe MySQL connection pool with configurable maximum size.
- Added RAII connection leases so DAO connections are returned automatically.
- Added connection validation, invalid-connection replacement, acquisition
  timeouts, and transaction-state cleanup when connections are returned.
- Added `CHATSERVER_DB_POOL_SIZE` and
  `CHATSERVER_DB_ACQUIRE_TIMEOUT_SECONDS` configuration options.

### Changed

- `Logger::createConnection()` now borrows from the pool instead of opening a
  new physical MySQL connection for every DAO operation.
- Startup now creates and validates the first pooled connection.
- UTF-8 compiler options are applied to migrated source files to preserve their
  existing Chinese text on Windows builds.
