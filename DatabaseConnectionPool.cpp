#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "DatabaseConnectionPool.h"

#include <chrono>
#include <cstdlib>
#include <stdexcept>
#include <utility>
#include <jdbc/mysql_driver.h>

namespace
{
std::string requireEnvironmentVariable(const char* name)
{
    const char* value = std::getenv(name);
    if (!value || *value == '\0')
        throw std::runtime_error(std::string("Missing environment variable: ") + name);
    return value;
}

std::size_t readPositiveSize(const char* name, std::size_t defaultValue)
{
    const char* value = std::getenv(name);
    if (!value || *value == '\0') return defaultValue;
    try
    {
        const auto parsed = std::stoull(value);
        if (parsed == 0)
            throw std::runtime_error(std::string(name) + " must be greater than zero");
        return static_cast<std::size_t>(parsed);
    }
    catch (const std::invalid_argument&)
    {
        throw std::runtime_error(std::string(name) + " must be a positive integer");
    }
    catch (const std::out_of_range&)
    {
        throw std::runtime_error(std::string(name) + " is too large");
    }
}
} // namespace

PooledConnection::PooledConnection(DatabaseConnectionPool* pool,
    std::unique_ptr<sql::Connection> connection) noexcept
    : pool_(pool), connection_(std::move(connection)) {}

PooledConnection::~PooledConnection() { release(); }

PooledConnection::PooledConnection(PooledConnection&& other) noexcept
    : pool_(other.pool_), connection_(std::move(other.connection_))
{
    other.pool_ = nullptr;
}

PooledConnection& PooledConnection::operator=(PooledConnection&& other) noexcept
{
    if (this != &other)
    {
        release();
        pool_ = other.pool_;
        connection_ = std::move(other.connection_);
        other.pool_ = nullptr;
    }
    return *this;
}

sql::Connection* PooledConnection::operator->() const noexcept { return connection_.get(); }

sql::Connection& PooledConnection::operator*() const
{
    if (!connection_)
        throw std::runtime_error("Attempted to use an empty database connection lease");
    return *connection_;
}

PooledConnection::operator bool() const noexcept { return connection_ != nullptr; }

void PooledConnection::release() noexcept
{
    if (pool_ && connection_) pool_->returnConnection(std::move(connection_));
    pool_ = nullptr;
}

DatabaseConnectionPool& DatabaseConnectionPool::instance()
{
    static DatabaseConnectionPool pool;
    return pool;
}

DatabaseConnectionPool::DatabaseConnectionPool() = default;

void DatabaseConnectionPool::initialize()
{
    //加入线程锁防止首次调用就有多个线程进行初始化所以加上锁
    std::lock_guard<std::mutex> lock(mutex_);
    //保证只有首次调用initialize才会进行参数初始化后面直接重复调用直接return
    if (initialized_) return;
   
    host_ = "45.197.144.95";
    user_ = "admin";
    password_ ="yexiang123";
    schema_ = "chatbase";
    maxConnections_ = readPositiveSize("CHATSERVER_DB_POOL_SIZE", 10);
    acquireTimeoutSeconds_ = static_cast<unsigned int>(
        readPositiveSize("CHATSERVER_DB_ACQUIRE_TIMEOUT_SECONDS", 10));

    // Fail fast during startup, then grow lazily as concurrency increases.
    idleConnections_.push(createPhysicalConnection());
    totalConnections_ = 1;
    initialized_ = true;
}
//从连接池中安全地借出一个数据库连接，并封装成 PooledConnection 返回。
// 如果没有空闲连接，则按需创建；达到连接数上限后等待，超过等待时间则抛出异常
PooledConnection DatabaseConnectionPool::acquire()
{
    initialize();
    std::unique_lock<std::mutex> lock(mutex_);
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::seconds(acquireTimeoutSeconds_);

    while (true)
    {
        if (!idleConnections_.empty())
        {
            //从队列头取出第一个连接进行使用
            auto connection = std::move(idleConnections_.front());
            idleConnections_.pop();
            lock.unlock();
            //若为无效连接则重新创建
            if (!isUsable(connection.get()))
            {
                try { connection = createPhysicalConnection(); }
                catch (...)
                {
                    std::lock_guard<std::mutex> countLock(mutex_);
                    --totalConnections_;
                    available_.notify_one();
                    throw;
                }
            }
            return PooledConnection(this, std::move(connection));
        }
        //若当前连接小于既定数则进行创建
        if (totalConnections_ < maxConnections_)
        {
            ++totalConnections_;
            lock.unlock();
            try { return PooledConnection(this, createPhysicalConnection()); }
            catch (...)
            {
                std::lock_guard<std::mutex> countLock(mutex_);
                --totalConnections_;
                available_.notify_one();
                throw;
            }
        }

        if (available_.wait_until(lock, deadline) == std::cv_status::timeout)
            throw std::runtime_error("Timed out waiting for a database connection");
    }
}

std::unique_ptr<sql::Connection> DatabaseConnectionPool::createPhysicalConnection() const
{
    auto* driver = sql::mysql::get_driver_instance();
    std::unique_ptr<sql::Connection> connection(driver->connect(host_, user_, password_));
    connection->setSchema(schema_);
    return connection;
}

bool DatabaseConnectionPool::isUsable(sql::Connection* connection) const noexcept
{
    if (!connection) return false;
    try { return !connection->isClosed() && connection->isValid(); }
    catch (...) { return false; }
}

void DatabaseConnectionPool::returnConnection(
    std::unique_ptr<sql::Connection> connection) noexcept
{
    if (!connection) return;
    try
    {
        if (!connection->getAutoCommit())
        {
            connection->rollback();
            connection->setAutoCommit(true);
        }
    }
    catch (...) { connection.reset(); }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (connection && isUsable(connection.get()))
            idleConnections_.push(std::move(connection));
        else
            --totalConnections_;
    }
    available_.notify_one();
}
