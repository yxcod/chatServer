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
    // 创建一个不持有数据库连接的空租约，主要用于延迟赋值或移动赋值。
    PooledConnection() noexcept = default;

    // 销毁租约，并将当前持有的数据库连接自动归还到连接池。
    ~PooledConnection();

    // 禁止复制，确保同一个数据库连接在同一时间只有一个租约负责管理。
    PooledConnection(const PooledConnection&) = delete;

    // 禁止复制赋值，避免两个租约同时管理并归还同一个数据库连接。
    PooledConnection& operator=(const PooledConnection&) = delete;

    // 通过移动构造转移连接租约的所有权；移动后原对象不再持有连接。
    PooledConnection(PooledConnection&& other) noexcept;

    // 通过移动赋值接管其他租约；接管前会先归还当前对象持有的连接。
    PooledConnection& operator=(PooledConnection&& other) noexcept;

    // 提供类似智能指针的访问方式，支持 con->prepareStatement(...) 等调用。
    sql::Connection* operator->() const noexcept;

    // 返回底层数据库连接的引用；空租约调用时会抛出异常。
    sql::Connection& operator*() const;

    // 判断当前租约是否持有有效的连接对象，不执行数据库连通性检查。
    explicit operator bool() const noexcept;

private:
    friend class DatabaseConnectionPool;

    // 仅供连接池创建租约，将物理连接的所有权交给当前对象管理。
    PooledConnection(DatabaseConnectionPool* pool,
                     std::unique_ptr<sql::Connection> connection) noexcept;

    // 将连接归还连接池并清空池指针；可重复调用且不会重复归还。
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
