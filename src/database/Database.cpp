#include "Database.h"
#include <iostream>
#include <mysql/mysql.h>

Database::Database(const std::string& host,
                   const std::string& user,
                   const std::string& pass,
                   const std::string& dbname)
    : host(host), user(user), pass(pass), dbname(dbname), conn(nullptr) {

    conn = mysql_init(nullptr);
    if (conn == nullptr) {
        throw std::runtime_error("MySQL initialization failed.");
    }

    if (mysql_real_connect(conn, host.c_str(), user.c_str(), pass.c_str(),
                           dbname.c_str(), 3306, nullptr, 0) == nullptr) {
        throw std::runtime_error("MySQL connection failed: " + std::string(mysql_error(conn)));
    }
}

Database::~Database() {
    if (conn != nullptr) {
        mysql_close(conn);
    }
}

MYSQL* Database::getConnection() {
    return conn;
}

// CRUD-Methoden für die 'benutzer' Tabelle

// Create: Einen neuen Benutzer erstellen
void Database::createUser(const std::string& username, const std::string& password) {
    MYSQL* conn = getConnection();
    std::string query = "INSERT INTO benutzer (username, password) VALUES ('" + username + "', '" + password + "')";
    
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Insert failed: " << mysql_error(conn) << std::endl;
    }
}

// Read: Alle Benutzer abfragen
void Database::getUsers() {
    MYSQL* conn = getConnection();
    std::string query = "SELECT * FROM benutzer";
    
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
        return;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (res == nullptr) {
        std::cerr << "Failed to store result: " << mysql_error(conn) << std::endl;
        return;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        std::cout << "ID: " << row[0] << ", Username: " << row[1] << std::endl;
    }

    mysql_free_result(res);
}

// Update: Einen Benutzer aktualisieren
void Database::updateUser(int id, const std::string& username, const std::string& password) {
    MYSQL* conn = getConnection();
    std::string query = "UPDATE benutzer SET username = '" + username + "', password = '" + password + "' WHERE id = " + std::to_string(id);
    
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Update failed: " << mysql_error(conn) << std::endl;
    }
}

// Delete: Einen Benutzer löschen
void Database::deleteUser(int id) {
    MYSQL* conn = getConnection();
    std::string query = "DELETE FROM benutzer WHERE id = " + std::to_string(id);
    
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Delete failed: " << mysql_error(conn) << std::endl;
    }
}

// CRUD-Methoden für die 'benachrichtigung' Tabelle (Beispiel)

// Alle Benachrichtigungen abfragen
void Database::getNotifications() {
    MYSQL* conn = getConnection();
    std::string query = "SELECT * FROM benachrichtigung";
    
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
        return;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (res == nullptr) {
        std::cerr << "Failed to store result: " << mysql_error(conn) << std::endl;
        return;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        std::cout << "Notification: " << row[1] << std::endl;
    }

    mysql_free_result(res);
}

// Weitere CRUD-Methoden für andere Tabellen (z.B. 'gebaeude', 'sensoren', etc.)
void Database::getBuildings() {
    MYSQL* conn = getConnection();
    std::string query = "SELECT * FROM gebaeude";
    
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
        return;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (res == nullptr) {
        std::cerr << "Failed to store result: " << mysql_error(conn) << std::endl;
        return;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        std::cout << "Building ID: " << row[0] << ", Building Name: " << row[1] << std::endl;
    }

    mysql_free_result(res);
}