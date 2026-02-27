#pragma once

#include <mysql/mysql.h>
#include <string>

class Database {
public:
    // Constructor und Destructor
    Database(const std::string& host,
             const std::string& user,
             const std::string& pass,
             const std::string& dbname);
    ~Database();

    // Connection Management
    MYSQL* getConnection();

    // User (Benutzer) CRUD Operations
    void createUser(const std::string& username, const std::string& password);
    void getUsers();
    void updateUser(int id, const std::string& username, const std::string& password);
    void deleteUser(int id);

    // Notification (Benachrichtigung) Operations
    void getNotifications();

    // Building (Gebäude) Operations
    void getBuildings();

private:
    MYSQL* conn;
    std::string host;
    std::string user;
    std::string pass;
    std::string dbname;
};