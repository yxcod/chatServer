# ChatServer

A C++17 instant-messaging server built with Drogon, MySQL Connector/C++, OpenSSL and Visual Studio 2022.

## Configuration

Set these environment variables before starting the server:

- `CHATSERVER_DB_HOST`
- `CHATSERVER_DB_USER`
- `CHATSERVER_DB_PASSWORD`
- `CHATSERVER_DB_SCHEMA`

The JWT signing secret is currently fixed in `UserLoginService.cpp`, so
`CHATSERVER_JWT_SECRET` is not required. Move it back to secure external
configuration before deploying the project publicly.

Optional database pool settings:

- `CHATSERVER_DB_POOL_SIZE` (default: `10`)
- `CHATSERVER_DB_ACQUIRE_TIMEOUT_SECONDS` (default: `10`)

See `.env.example` for local development placeholders. Never commit real credentials.

## Build

Open `ChatServer.sln` with Visual Studio 2022 and build the x64 configuration. The project currently expects Boost 1.86 at `C:\boost_1_86_0`.

By default, the HTTP/WebSocket service listens on port `5555`.

# Logging and crash reports

ChatServer writes rotating runtime logs to `logs/` next to `ChatServer.exe`.
Each runtime log is limited to 20 MB and the latest 10 files are retained.

Unhandled Windows exceptions, fatal signals, and uncaught C++ exceptions create
files in `logs/crashes/`:

- `crash-<time>-p<process>-t<thread>.log`: exception details and stack trace.
- `crash-<time>-p<process>-t<thread>.dmp`: Windows minidump for Visual Studio or WinDbg.

Keep the matching `.pdb` file beside the deployed executable. When reporting a
crash, first provide the crash `.log` and the latest `chat-server` runtime log.
If the text stack is incomplete, also provide the matching `.dmp`, `.pdb`, and
the Git commit ID.
