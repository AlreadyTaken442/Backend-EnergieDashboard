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

class HttpServer {
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
        acceptor_.async_accept(
            net::make_strand(ioc_),
            [this](beast::error_code ec, tcp::socket socket) {
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

                // Verarbeite die Anfrage und sende die Antwort
                handle_request();
            });
        }

        void handle_request() {
            http::response<http::string_body> res{http::status::ok, request_.version()};

            if (request_.method() == http::verb::get) {
                controller_.handleGetUsers(request_, res);
            } else if (request_.method() == http::verb::post) {
                controller_.handleCreateUser(request_, res);
            } else if (request_.method() == http::verb::put) {
                controller_.handleUpdateUser(request_, res);
            } else if (request_.method() == http::verb::delete_) {
                controller_.handleDeleteUser(request_, res);
            }

            http::async_write(socket_, res, [this](beast::error_code ec, std::size_t) {
                if (ec) {
                    std::cerr << "Write failed: " << ec.message() << std::endl;
                }
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
