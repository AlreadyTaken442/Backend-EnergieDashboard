// src/controllers/HttpController.h
#ifndef HTTP_CONTROLLER_H
#define HTTP_CONTROLLER_H

#include <boost/beast/core.hpp>        // beast basis
#include <boost/beast/http.hpp>        // http::request/response
#include <string>

namespace beast = boost::beast;
namespace http  = beast::http;

class Database;   // forward declaration, der Header wird in der .cpp eingebunden

/// HTTP-Controller, der CRUD‑Operationen über die Database ausführt.
class HttpController {
public:
    explicit HttpController(Database& db);

    // Request‑Handler (jeweils req & res werden vom Aufrufer angelegt)
    void handleGetUsers(http::request<http::string_body>& req,
                        http::response<http::string_body>& res);
    void handleCreateUser(http::request<http::string_body>& req,
                          http::response<http::string_body>& res);
    void handleUpdateUser(http::request<http::string_body>& req,
                          http::response<http::string_body>& res);
    void handleDeleteUser(http::request<http::string_body>& req,
                          http::response<http::string_body>& res);

private:
    Database& db_;
};

#endif // HTTP_CONTROLLER_H