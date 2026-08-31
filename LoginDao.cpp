#include "LoginDao.h"
#include "Logger.h"

int LoginDao::registerAccount(const std::string& account, const std::string& password, const std::string& salt)
{
    // 每次调用创建独立连接
    auto con = Logger::GetInstance().createConnection();

    try
    {
        // 先检查是否存在
        std::string selectSql = "SELECT id FROM login WHERE userAccount = ?";
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(selectSql));
        pstmt->setString(1, account);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (res->rowsCount() > 0)
        {
            // 账号已存在
            return -1;
        }

        // 插入新账号
        std::string insertSql =
            "INSERT INTO login (userAccount, password, isBan, salt) "
            "VALUES (?, ?, ?, ?)";

        pstmt.reset(con->prepareStatement(insertSql));
        pstmt->setString(1, account);
        pstmt->setString(2, password);
        pstmt->setInt(3, 0);   // 注册时默认未封禁
        pstmt->setString(4, salt);

        int resultValue = pstmt->executeUpdate();
        return resultValue;
    }
    catch (...)
    {
        return 0;
    }
}

// 建议：不要再返回 ResultSet*，而是封装成一个结构体/DTO；
// 这里给出一个示例实现：调用方只拿到需要的字段。
LoginInfo LoginDao::loginAccount(const std::string& account)
{
    LoginInfo info{};
    info.found = false;

    auto con = Logger::GetInstance().createConnection();

    try
    {
        std::string selectSql =
            "SELECT password, isBan, salt FROM login WHERE userAccount = ?";
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(selectSql));
        pstmt->setString(1, account);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (res->next())
        {
            info.found = true;
            info.password = res->getString("password");
            info.isBan = res->getInt("isBan");
            info.salt = res->getString("salt");
        }
    }
    catch (...)
    {
        // 出错就保持 found=false
    }

    return info;
}

bool LoginDao::isAccountActive(const std::string& account) const
{
    if (account.empty()) return false;
    auto con = Logger::GetInstance().createConnection();
    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(
                "SELECT 1 FROM login WHERE userAccount = ? AND isBan = 0 LIMIT 1"));
        pstmt->setString(1, account);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        return res->next();
    }
    catch (...)
    {
        return false;
    }
}

int LoginDao::changePassword(const std::string& account, const std::string& newPassword)
{
    auto con = Logger::GetInstance().createConnection();

    try
    {
        std::string updateSql =
            "UPDATE login SET password = ? WHERE userAccount = ?";
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->prepareStatement(updateSql));
        pstmt->setString(1, newPassword);
        pstmt->setString(2, account);
        return pstmt->executeUpdate();
    }
    catch (...)
    {
        return 0;
    }
}
