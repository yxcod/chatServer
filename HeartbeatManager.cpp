#include "HeartbeatManager.h"
#include "Logger.h"
#include "UserInfoDao.h"
#include "UserModel.h"
#include "ChatManageController.h"
HeartbeatManager& HeartbeatManager::GetInstance()
{
    static HeartbeatManager instance;
    return instance;
}

HeartbeatManager::HeartbeatManager() = default;
HeartbeatManager::~HeartbeatManager() = default;

void HeartbeatManager::handleHeartbeat(const std::string& userName)
{
    if (userName.empty())
    {
        return;
    }

    const uint64_t now = Logger::GetInstance().getcurrentTime();
    // 在线和离线数据库写入共用同一把锁，保证快速断线重连时写入顺序正确。
    std::lock_guard<std::mutex> lock(m_hbMutex);
    m_lastHeartbeat[userName] = now;
    if (m_lastDbOnline.find(userName) == m_lastDbOnline.end())
    {
        m_lastDbOnline[userName] = now;
        UserInfoDao userInfoDao;
        UserInfo userInfo;
        userInfo.setState(1); // 在线
        userInfo.setModifyTime(now);
        userInfoDao.updateUserInfo(userName, userInfo);
    }
}

void HeartbeatManager::handleDisconnect(const std::string& userName)
{
    if (userName.empty()) return;

    // 数据库更新放在同一把锁内，避免旧连接的离线更新覆盖刚重连的在线更新。
    std::lock_guard<std::mutex> lock(m_hbMutex);
    m_lastHeartbeat.erase(userName);
    m_lastDbOnline.erase(userName);

    UserInfo offlineUser;
    offlineUser.setState(0);
    offlineUser.setModifyTime(Logger::GetInstance().getcurrentTime());
    UserInfoDao().updateUserInfo(userName, offlineUser);
}

void HeartbeatManager::checkHeartbeatTimeout()
{
    const uint64_t now = Logger::GetInstance().getcurrentTime();
    UserInfoDao userInfoDao;

    std::vector<std::string> timeoutUsers;

    {
        std::lock_guard<std::mutex> lock(m_hbMutex);
        for (auto it = m_lastHeartbeat.begin(); it != m_lastHeartbeat.end();)
        {
            if (now - it->second > m_timeoutSeconds * 1000)
            {
                UserInfo offlineUser;
                offlineUser.setState(0);
                offlineUser.setModifyTime(now);

                const std::string userName = it->first;

                if (userInfoDao.updateUserInfo(userName, offlineUser) > 0)
                {
                    timeoutUsers.push_back(userName);

                    // 从心跳表中删除
                    it = m_lastHeartbeat.erase(it);

                    // 关键：同时从“已写在线”表中删除，
                    // 这样下次这个用户有心跳时，会再次 needUpdateDb=true，重新写 state=1
                    m_lastDbOnline.erase(userName);
                }
                else
                {
                    ++it;
                }
            }
            else
            {
                ++it;
            }
        }
    }

    ChatWSServer* wsServer = ChatWSServer::GetInstance();
    if (wsServer != nullptr)
    {
        for (const auto& userName : timeoutUsers)
        {
            wsServer->closeConnectionByUser(userName);
        }
    }
}
