#include "HttpController.h" // Zuerst deinen Header inkludieren!
#include "../database/Database.h"
#include <iostream>
#include <sstream>

HttpController::HttpController(Database& db) : db_(db) {}

// 2. Die Funktionen mit HttpController:: davor
void HttpController::handleGetUsers(http::request<http::string_body>& req, 
                                    http::response<http::string_body>& res) {
    std::stringstream response_body;
    db_.getUsers(); 
    response_body << "GET request: List of users (example response)\n";
    res.set(http::field::content_type, "text/plain");
    res.body() = response_body.str();
    res.prepare_payload();
}

void HttpController::handleCreateUser(http::request<http::string_body>& req, 
                                      http::response<http::string_body>& res) {
    db_.createUser("new_user", "password123");
    res.result(http::status::created);
    res.set(http::field::content_type, "text/plain");
    res.body() = "User created successfully!";
    res.prepare_payload();
}

void HttpController::handleUpdateUser(http::request<http::string_body>& req, 
                                      http::response<http::string_body>& res) {
    db_.updateUser(1, "updated_user", "new_password");
    res.result(http::status::ok);
    res.set(http::field::content_type, "text/plain");
    res.body() = "User updated successfully!";
    res.prepare_payload();
}

void HttpController::handleDeleteUser(http::request<http::string_body>& req, 
                                      http::response<http::string_body>& res) {
    db_.deleteUser(1);
    res.result(http::status::ok);
    res.set(http::field::content_type, "text/plain");
    res.body() = "User deleted successfully!";
    res.prepare_payload();
}
