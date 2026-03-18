#ifndef HTTP_CONTROLLER_H
#define HTTP_CONTROLLER_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <string>

namespace beast = boost::beast;
namespace http = beast::http;

class Database;

class HttpController {
public:
    explicit HttpController(Database& db);

    void handleListResource(const std::string& resourceName,
                            http::response<http::string_body>& res);
    void handleCreateUser(http::request<http::string_body>& req,
                          http::response<http::string_body>& res);
    void handleUpdateUser(http::request<http::string_body>& req,
                          http::response<http::string_body>& res);
    void handleDeleteUser(http::request<http::string_body>& req,
                          http::response<http::string_body>& res);
    void handleLogin(http::request<http::string_body>& req,
                     http::response<http::string_body>& res);

private:
    Database& db_;
};

#endif
