# ChatServer

A C++17 instant-messaging server built with Drogon, MySQL Connector/C++, OpenSSL and Visual Studio 2022.

## Configuration

Set these environment variables before starting the server:

- `CHATSERVER_DB_HOST`
- `CHATSERVER_DB_USER`
- `CHATSERVER_DB_PASSWORD`
- `CHATSERVER_DB_SCHEMA`
- `CHATSERVER_JWT_SECRET`

See `.env.example` for local development placeholders. Never commit real credentials.

## Build

Open `ChatServer.sln` with Visual Studio 2022 and build the x64 configuration. The project currently expects Boost 1.86 at `C:\boost_1_86_0`.

By default, the HTTP/WebSocket service listens on port `5555`.
