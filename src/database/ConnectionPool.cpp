#pragma once
#include <mysql/mysql.h>
#include <vector>
#include <mutex>
#include <string>

class ConnectionPool {
public:
    ConnectionPool(const std::string& host,
                   const std::string& user,
                   const std::string& pass,
                   const std::string& dbname,
                   int poolSize);

    ~ConnectionPool();

    MYSQL* getConnection();
    void releaseConnection(MYSQL* connection);

private:
    std::vector<MYSQL*> pool;
    std::mutex poolMutex;
    std::string host, user, pass, dbname;
};