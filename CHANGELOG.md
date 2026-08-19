# Changelog

All notable project changes are documented in this file.

## [Unreleased]

### Documentation

- Added Chinese usage comments for every `PooledConnection` member function,
  including ownership transfer, connection access, and automatic pool return.
- Established the convention that newly added or modified functions require
  purpose comments, with additional explanation for complex logic.

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
