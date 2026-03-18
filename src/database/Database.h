#pragma once

#include <mysql/mysql.h>

#include <optional>
#include <string>

struct UserAuthRecord {
    int userId;
    std::string name;
    std::string email;
    std::string passwordHash;
    bool active;
};

class Database {
public:
    Database(const std::string& host,
             const std::string& user,
             const std::string& pass,
             const std::string& dbname);
    ~Database();

    MYSQL* getConnection();

    bool createUser(const std::string& name,
                    const std::string& email,
                    const std::string& passwordHash);
    bool userExistsByEmail(const std::string& email);
    std::optional<UserAuthRecord> getUserAuthByEmail(const std::string& email);
    bool updateUser(int id,
                    const std::string& name,
                    const std::string& email,
                    const std::string& passwordHash,
                    bool active);
    bool deleteUser(int id);
    bool getRoomDevices(const std::string& roomId);

    std::optional<std::string> getTableRowsAsJson(const std::string& tableName);

private:
    MYSQL* conn;
    std::string host;
    std::string user;
    std::string pass;
    std::string dbname;
};
