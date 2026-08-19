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
    bool needUpdateDb = false;

    {
        std::lock_guard<std::mutex> lock(m_hbMutex);

        // 更新最近一次心跳时间（总是更新，仅在内存）
        m_lastHeartbeat[userName] = now;

        // 如果之前从未把该用户写为在线，则本次需要写数据库
        auto it = m_lastDbOnline.find(userName);
        if (it == m_lastDbOnline.end())
        {
            needUpdateDb = true;
            m_lastDbOnline[userName] = now;
        }
    }

    // 只有在需要时才更新数据库，避免每次心跳都写
    if (needUpdateDb)
    {
        UserInfoDao userInfoDao;
        UserInfo userInfo;
        userInfo.setState(1); // 在线
        userInfo.setModifyTime(now);
        userInfoDao.updateUserInfo(userName, userInfo);
    }
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