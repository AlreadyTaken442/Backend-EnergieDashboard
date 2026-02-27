#include "MessageRepository.h"
#include "./database/Database.h"
#include <mysql/mysql.h>
#include <iostream>

MessageRepository::MessageRepository(Database& db)
    : db(db) {}

void MessageRepository::addMessage(const std::string& messageContent) {
    MYSQL* conn = db.getConnection();
    // Bereite SQL-Query vor und führe sie aus
    std::string query = "INSERT INTO messages (content) VALUES ('" + messageContent + "')";
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
    }
}