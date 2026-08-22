#pragma once
#include <unordered_map>
#include <mutex>
#include <string>

class HeartbeatManager
{
public:
    // 单例获取
    static HeartbeatManager& GetInstance();

    // 前端每隔几秒调用一次，传入 userName，记录心跳并标记在线
    void handleHeartbeat(const std::string& userName);

    // WebSocket 断开时立即清理状态，保证后续重连可以重新写入在线状态。
    void handleDisconnect(const std::string& userName);

    // 由服务器定时器周期性调用，检查心跳超时并更新离线
    void checkHeartbeatTimeout();

private:
    HeartbeatManager();
    ~HeartbeatManager();

    HeartbeatManager(const HeartbeatManager&) = delete;
    HeartbeatManager& operator=(const HeartbeatManager&) = delete;
    HeartbeatManager(HeartbeatManager&&) = delete;
    HeartbeatManager& operator=(HeartbeatManager&&) = delete;

private:
    std::unordered_map<std::string, uint64_t> m_lastHeartbeat;
    // 最近一次已写入数据库为在线(state=1)的时间
    std::unordered_map<std::string, uint64_t> m_lastDbOnline;
    std::mutex m_hbMutex;
    static constexpr uint64_t m_timeoutSeconds = 30; // 超过20秒视为离线
};
