#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <iostream>
#include <memory>
#include <string>
#include "./controllers/HttpController.h"
#include "./database/Database.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class HttpServer : public std::enable_shared_from_this<HttpServer> {
public:
    HttpServer(net::io_context& ioc, tcp::endpoint endpoint, Database& db)
        : ioc_(ioc), acceptor_(net::make_strand(ioc)), controller_(db) {
        beast::error_code ec;

        // Öffne den Acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            std::cerr << "Open error: " << ec.message() << std::endl;
            return;
        }

        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if (ec) {
            std::cerr << "Set option error: " << ec.message() << std::endl;
            return;
        }

        acceptor_.bind(endpoint, ec);
        if (ec) {
            std::cerr << "Bind error: " << ec.message() << std::endl;
            return;
        }

        acceptor_.listen(net::socket_base::max_listen_connections, ec);
        if (ec) {
            std::cerr << "Listen error: " << ec.message() << std::endl;
            return;
        }
    }

    void run() {
        do_accept();
    }

private:
    void do_accept() {
        auto self = shared_from_this();
        acceptor_.async_accept(
            net::make_strand(ioc_),
            [self, this](beast::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<Session>(std::move(socket), controller_)->run();
                }
                do_accept();
            });
    }

    class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(tcp::socket socket, HttpController& controller)
            : socket_(std::move(socket)), controller_(controller) {}

        void run() {
            do_read();
        }

    private:
        void do_read() {
            auto self(shared_from_this());
            http::async_read(socket_, buffer_, request_,
                             [this, self](beast::error_code ec, std::size_t) {
                if (ec == http::error::end_of_stream) {
                    beast::error_code ignored;
                    socket_.shutdown(tcp::socket::shutdown_send, ignored);
                    return;
                }
                if (ec) {
                    std::cerr << "Read failed: " << ec.message() << std::endl;
                    return;
                }

                // Verarbeite die Anfrage und sende die Antwort
                handle_request();
            });
        }

        void handle_request() {
            http::response<http::string_body> res{http::status::ok, request_.version()};

            const std::string target = std::string(request_.target());

            if (request_.method() == http::verb::post && target == "/auth/register") {
                controller_.handleCreateUser(request_, res);
            } else if (request_.method() == http::verb::post && target == "/auth/login") {
                controller_.handleLogin(request_, res);
            } else if (request_.method() == http::verb::get && target == "/users") {
                controller_.handleListResource("users", res);
            } else if (request_.method() == http::verb::get && target == "/notifications") {
                controller_.handleListResource("notifications", res);
            } else if (request_.method() == http::verb::get && target == "/user-roles") {
                controller_.handleListResource("user-roles", res);
            } else if (request_.method() == http::verb::get && target == "/permissions") {
                controller_.handleListResource("permissions", res);
            } else if (request_.method() == http::verb::get && target == "/buildings") {
                controller_.handleListResource("buildings", res);
            } else if (request_.method() == http::verb::get && target == "/devices") {
                controller_.handleListResource("devices", res);
            } else if (request_.method() == http::verb::get && target == "/device-types") {
                controller_.handleListResource("device-types", res);
            } else if (request_.method() == http::verb::get && target == "/usage-statistics") {
                controller_.handleListResource("usage-statistics", res);
            } else if (request_.method() == http::verb::get && target == "/rooms") {
                controller_.handleListResource("rooms", res);
            } else if (request_.method() == http::verb::get && target == "/roles") {
                controller_.handleListResource("roles", res);
            } else if (request_.method() == http::verb::get && target == "/role-permissions") {
                controller_.handleListResource("role-permissions", res);
            } else if (request_.method() == http::verb::get && target == "/sensor-data") {
                controller_.handleListResource("sensor-data", res);
            } else if (request_.method() == http::verb::put && target == "/users") {
                controller_.handleUpdateUser(request_, res);
            } else if (request_.method() == http::verb::delete_ && target == "/users") {
                controller_.handleDeleteUser(request_, res);
            } else {
                res.result(http::status::not_found);
                res.set(http::field::content_type, "application/json");
                res.body() = "{\"error\":\"route not found\"}";
                res.prepare_payload();
            }

            auto self(shared_from_this());
            http::async_write(socket_, *res, [this, self, res](beast::error_code ec, std::size_t) {
                if (ec) {
                    std::cerr << "Write failed: " << ec.message() << std::endl;
                    return;
                }

                beast::error_code ignored;
                socket_.shutdown(tcp::socket::shutdown_send, ignored);
            });
        }

        tcp::socket socket_;
        beast::flat_buffer buffer_;
        http::request<http::string_body> request_;
        HttpController& controller_;
    };

    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    HttpController controller_;
};
