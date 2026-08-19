#pragma once

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <jdbc/cppconn/connection.h>

class DatabaseConnectionPool;

class PooledConnection
{
public:
    PooledConnection() noexcept = default;
    ~PooledConnection();
    PooledConnection(const PooledConnection&) = delete;
    PooledConnection& operator=(const PooledConnection&) = delete;
    PooledConnection(PooledConnection&& other) noexcept;
    PooledConnection& operator=(PooledConnection&& other) noexcept;

    sql::Connection* operator->() const noexcept;
    sql::Connection& operator*() const;
    explicit operator bool() const noexcept;

private:
    friend class DatabaseConnectionPool;
    PooledConnection(DatabaseConnectionPool* pool,
                     std::unique_ptr<sql::Connection> connection) noexcept;
    void release() noexcept;

    DatabaseConnectionPool* pool_{nullptr};
    std::unique_ptr<sql::Connection> connection_;
};

class DatabaseConnectionPool
{
public:
    static DatabaseConnectionPool& instance();
    DatabaseConnectionPool(const DatabaseConnectionPool&) = delete;
    DatabaseConnectionPool& operator=(const DatabaseConnectionPool&) = delete;

    PooledConnection acquire();
    void initialize();

private:
    friend class PooledConnection;
    DatabaseConnectionPool();
    ~DatabaseConnectionPool() = default;

    std::unique_ptr<sql::Connection> createPhysicalConnection() const;
    void returnConnection(std::unique_ptr<sql::Connection> connection) noexcept;
    bool isUsable(sql::Connection* connection) const noexcept;

    std::string host_;
    std::string user_;
    std::string password_;
    std::string schema_;
    std::size_t maxConnections_{10};
    unsigned int acquireTimeoutSeconds_{10};
    std::mutex mutex_;
    std::condition_variable available_;
    std::queue<std::unique_ptr<sql::Connection>> idleConnections_;
    std::size_t totalConnections_{0};
    bool initialized_{false};
};
