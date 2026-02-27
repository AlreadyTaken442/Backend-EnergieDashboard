#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <memory>
#include <iostream>
#include <exception>
#include "./database/Database.h"  // Deine Datenbankklasse
#include "./controllers/HttpController.h"  // Der Controller für HTTP-Anfragen (CRUD-Operationen)

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = boost::beast::http;
using tcp = boost::asio::ip::tcp;

// Listener-Klasse für den HTTP-Server (mit REST-Endpunkten)
class Listener : public std::enable_shared_from_this<Listener> {
public:
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    Database& db_;  // Referenz auf die Datenbankverbindung

public:
    Listener(net::io_context& ioc, tcp::endpoint endpoint, Database& db)
        : ioc_(ioc), acceptor_(net::make_strand(ioc)), db_(db) {
        beast::error_code ec;

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
                    // Erstelle eine Session für die eingehende Verbindung
                    std::make_shared<Session>(std::move(socket), db_)->run();
                }
                do_accept();
            });
    }

    class Session : public std::enable_shared_from_this<Session> {
        tcp::socket socket_;
        beast::flat_buffer buffer_;
        http::request<http::string_body> req_;
        Database& db_;  // Referenz auf die Datenbank

    public:
        Session(tcp::socket socket, Database& db) 
            : socket_(std::move(socket)), db_(db) {}

        void run() {
            do_read();
        }

    private:
        void do_read() {
            auto self(shared_from_this());
            http::async_read(socket_, buffer_, req_,
                             [this, self](beast::error_code ec, std::size_t) {
                if (ec == http::error::end_of_stream) {
                    beast::error_code ignored;
                    socket_.shutdown(tcp::socket::shutdown_send, ignored);
                    return;
                }
                // Handle the request (CRUD operations based on the HTTP method)
                handle_request();
            });
        }

        void handle_request() {
            http::response<http::string_body> res{http::status::ok, req_.version()};

            HttpController controller(db_);  // Controller mit CRUD-Operationen

            // Handle requests based on HTTP method (GET, POST, PUT, DELETE)
            if (req_.method() == http::verb::get) {
                controller.handleGetUsers(req_, res);
            } else if (req_.method() == http::verb::post) {
                controller.handleCreateUser(req_, res);
            } else if (req_.method() == http::verb::put) {
                controller.handleUpdateUser(req_, res);
            } else if (req_.method() == http::verb::delete_) {
                controller.handleDeleteUser(req_, res);
            }

            // Antwort an den Client senden
            http::async_write(socket_, res, [this](beast::error_code ec, std::size_t) {
                if (ec) {
                    std::cerr << "Write failed: " << ec.message() << std::endl;
                }
            });
        }
    };
};

int main() {
    try {
        auto const address = net::ip::make_address("0.0.0.0");  // Alle Interfaces
        unsigned short port = 8080;

        net::io_context ioc{1};

        // Datenbankverbindung erstellen
        Database db("127.0.0.1", "dashboard_user", "MeinSicheresPasswort", "energiedashboard");

        // Listener für den HTTP-Server starten
        std::make_shared<Listener>(ioc, tcp::endpoint{address, port}, db)->run();

        ioc.run();  // Event Loop starten, um Anfragen zu bearbeiten
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}