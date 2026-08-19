#pragma once
#include "HeartbeatManager.h"
#include "DatabaseConnectionPool.h"
#include <iostream>
#include <string>
#include <ctime>  // 用于时间类型（数据库 DATETIME 对应 C++ tm 结构体）
#include <sstream>
#include <iomanip>
#include <drogon/HttpAppFramework.h>
#include <drogon/orm/DbClient.h>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/exception.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <nlohmann/json.hpp>

using namespace drogon;
using namespace std;
using json = nlohmann::json;

class Logger
{
public:
    // 禁用拷贝、赋值、移动
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    // 获取单例实例（无参，因为启动时已初始化）
    static Logger& GetInstance();
    // 从连接池借用连接；租约离开作用域时自动归还。
    PooledConnection createConnection() const;

    sql::ResultSet* executeSelectSql(const std::string& sql);
    int executeUpdateSql(const std::string& sql);

    sql::Statement* getStatement() const;
    sql::Connection* getConnection() const;

    // 初始化数据库
    void initMysSql();

    // 初始化服务器
    void initService();

    // 打印 json
    void debugJson(const Json::Value& j, const std::string& prefix = "");

    // 获取当前时间戳
    uint64_t getcurrentTime() const;
    //错误日志接口（支持 std::string / C 字符串）
    void error(const std::string& msg);
    // 将字符串数组拼接成 "s1_s2_s3" 形式
    std::string joinWithUnderscore(const std::vector<std::string>& items) const;

    // 从 "s1_s2_s3" 中移除指定子串（如移除 "s2" -> "s1_s3"）
    // 若格式不符合或不包含该子串，则返回原字符串
    std::string removeFromJoined(const std::string& joined, const std::string& toRemove) const;
    //创建目录
    void createDataDirectories(const std::string& name) const;
   
private:
    sql::Statement* dbPtr;
    sql::Connection* con;

    Logger();
    ~Logger();
};
