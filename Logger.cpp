#include "Logger.h"
#include "CrashHandler.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace
{
bool isSensitiveLogField(std::string field)
{
    std::transform(field.begin(), field.end(), field.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return field.find("password") != std::string::npos ||
           field.find("token") != std::string::npos ||
           field.find("authorization") != std::string::npos ||
           field.find("secret") != std::string::npos;
}
}

Logger& Logger::GetInstance()
{
    static Logger instance; // C++11+ 线程安全单例
    return instance;
}

PooledConnection Logger::createConnection() const
{
    return DatabaseConnectionPool::instance().acquire();
}

Logger::Logger()
    : dbPtr(nullptr),
      con(nullptr)
{
}

Logger::~Logger()
{
    // 这里根据需要决定是否关闭连接/释放资源
    // 例如：
    // if (dbPtr) { delete dbPtr; dbPtr = nullptr; }
    // if (con) { delete con; con = nullptr; }
}

sql::ResultSet* Logger::executeSelectSql(const std::string& sql)
{
    return dbPtr->executeQuery(sql);
}

int Logger::executeUpdateSql(const std::string& sql)
{
    return dbPtr->executeUpdate(sql);
}

sql::Statement* Logger::getStatement() const
{
    return dbPtr;
}

sql::Connection* Logger::getConnection() const
{
    return con;
}

// 初始化数据库
void Logger::initMysSql()
{
    DatabaseConnectionPool::instance().initialize();
}

// 初始化服务器
void Logger::initService()
{
    // 运行服务器
    try
    {
        drogon::app().addListener("0.0.0.0", 5555);
        // 为 300MB 视频预留少量 multipart 边界开销；控制器仍严格限制
        // 单个视频文件不得超过 300MB。超过内存阈值的请求体由 Drogon
        // 写入临时文件，避免上传时长期占用同等大小的内存。
        drogon::app()
            .setClientMaxBodySize(302ULL * 1024 * 1024)
            .setClientMaxMemoryBodySize(1024 * 1024);
    }
    catch (const std::exception& e)
    {
        error(std::string("addListener failed: ") + e.what());
        throw;
    }

    auto& app = drogon::app();
    auto* loop = app.getLoop();
    // 每 5 秒检查一次心跳超时（你可以根据需要调整周期）
    loop->runEvery(5.0, []() {
        HeartbeatManager::GetInstance().checkHeartbeatTimeout();
    });

    drogon::app().registerHandler(
        "/hello",
        [](const drogon::HttpRequestPtr& req,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setBody("Hello Drogon");
            callback(resp);
        },
        {drogon::Post, drogon::Get});

    std::cout << "Server running at http://127.0.0.1:5555/hello\n";
    // Some networking/runtime libraries replace process-wide handlers during
    // initialization. Reinstall immediately before entering the event loop.
    CrashHandler::install(CrashHandler::defaultLogDirectory());
    info(std::string("Crash reports directory: ") +
         (CrashHandler::defaultLogDirectory() / "crashes").u8string());
    drogon::app().run();
}

void Logger::initLogging(const std::string& logDirectory)
{
    std::filesystem::create_directories(logDirectory);
    drogon::app()
        .setLogPath(logDirectory, "chat-server", 20 * 1024 * 1024, 10, false)
        .setLogLocalTime(true)
        .setLogLevel(trantor::Logger::kDebug);
}

// 打印 json
void Logger::debugJson(const Json::Value& j, const std::string& prefix)
{
    if (j.isObject())
    {
        for (auto it = j.begin(); it != j.end(); ++it)
        {
            std::string key = prefix.empty() ? it.name() : prefix + "." + it.name();
            debugJson(*it, key);
        }
    }
    else if (j.isArray())
    {
        for (Json::ArrayIndex i = 0; i < j.size(); ++i)
        {
            std::string key = prefix + "[" + std::to_string(i) + "]";
            debugJson(j[i], key);
        }
    }
    else
    {
        // 避免将密码、令牌等敏感请求字段写入持久化日志。
        LOG_DEBUG << prefix << " = "
                  << (isSensitiveLogField(prefix) ? "[REDACTED]"
                                                  : j.toStyledString());
    }
}

// 获取当前时间戳
uint64_t Logger::getcurrentTime() const
{
    using namespace std::chrono;
    auto now = system_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count();
    return static_cast<uint64_t>(ms);
}

void Logger::error(const std::string& msg)
{
    LOG_ERROR << msg;
}

void Logger::info(const std::string& msg)
{
    LOG_INFO << msg;
}

void Logger::warning(const std::string& msg)
{
    LOG_WARN << msg;
}

std::string Logger::joinWithUnderscore(const std::vector<std::string>& items) const
{
    if (items.empty())
    {
        return {};
    }
    std::ostringstream os;
    for (std::size_t i = 0; i < items.size(); ++i)
    {
        if (i > 0)
        {
            os << '_';
        }
        os << items[i];
    }
    return os.str();
}

std::string Logger::removeFromJoined(const std::string& joined, const std::string& toRemove) const
{
    if (joined.empty() || toRemove.empty())
    {
        return joined;
    }

    // 简单格式校验：必须包含 '_'，且首尾不能是 '_'
    if (joined.front() == '_' || joined.back() == '_')
    {
        return joined;
    }
    std::vector<std::string> parts;
    parts.reserve(8);
    std::string current;
    for (char ch : joined)
    {
        if (ch == '_')
        {
            if (current.empty())
            {
                // 出现连续 '_'，认为格式非法，直接返回原值
                return joined;
            }
            parts.push_back(current);
            current.clear();
        }
        else
        {
            current.push_back(ch);
        }
    }
    if (current.empty())
    {
        // 结尾为空，格式非法
        return joined;
    }
    parts.push_back(current);

    // 过滤掉等于 toRemove 的项
    std::vector<std::string> filtered;
    filtered.reserve(parts.size());
    for (const auto& p : parts)
    {
        if (p != toRemove)
        {
            filtered.push_back(p);
        }
    }

    // 如果没有移除任何元素，则返回原串
    if (filtered.size() == parts.size())
    {
        return joined;
    }

    // 重新拼接
    return joinWithUnderscore(filtered);
}

void Logger::createDataDirectories(const std::string& name) const
{
    namespace fs = std::filesystem;

    // 当前工作目录
    const fs::path base = fs::current_path();

    const fs::path imageBase = base / "imageData";
    const fs::path videoBase = base / "videoData";
    const fs::path fileBase = base / "fileData";
    const fs::path groupResourceBase = base / "groupResourceData";

    // 先确保三个基础目录存在（不存在就创建）
    fs::create_directories(imageBase);
    fs::create_directories(videoBase);
    fs::create_directories(fileBase);
    fs::create_directories(groupResourceBase);

    // 在每个基础目录下创建以 name 命名的子目录（已存在不会报错）
    fs::create_directories(imageBase / name);
    fs::create_directories(videoBase / name);
    fs::create_directories(fileBase / name);
    
}
