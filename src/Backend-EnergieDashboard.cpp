#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <memory>
#include <iostream>
#include <exception>
#include <thread>
#include "./database/Database.h"  // Deine Datenbankklasse
#include "./controllers/HttpController.h"  // Der Controller für HTTP-Anfragen (CRUD-Operationen)
#include "./mqtt/MqttClient.h"

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
        auto self = shared_from_this();
        acceptor_.async_accept(
            net::make_strand(ioc_),
            [self, this](beast::error_code ec, tcp::socket socket) {
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
                if (ec) {
                    std::cerr << "Read failed: " << ec.message() << std::endl;
                    return;
                }
                // Handle the request (CRUD operations based on the HTTP method)
                handle_request();
            });
        }

        void handle_request() {
            auto res = std::make_shared<http::response<http::string_body>>(
                http::status::ok,
                req_.version());

            HttpController controller(db_);  // Controller mit CRUD-Operationen

            const std::string target = std::string(req_.target());

            if (req_.method() == http::verb::post && target == "/auth/register") {
                controller.handleCreateUser(req_, *res);
            } else if (req_.method() == http::verb::post && target == "/auth/login") {
                controller.handleLogin(req_, *res);
            } else if (req_.method() == http::verb::get && target == "/users") {
                controller.handleListResource("users", *res);
            } else if (req_.method() == http::verb::get && target == "/notifications") {
                controller.handleListResource("notifications", *res);
            } else if (req_.method() == http::verb::get && target == "/user-roles") {
                controller.handleListResource("user-roles", *res);
            } else if (req_.method() == http::verb::get && target == "/permissions") {
                controller.handleListResource("permissions", *res);
            } else if (req_.method() == http::verb::get && target == "/buildings") {
                controller.handleListResource("buildings", *res);
            } else if (req_.method() == http::verb::get && target == "/devices") {
                controller.handleListResource("devices", *res);
            } else if (req_.method() == http::verb::get && target == "/device-types") {
                controller.handleListResource("device-types", *res);
            } else if (req_.method() == http::verb::get && target == "/usage-statistics") {
                controller.handleListResource("usage-statistics", *res);
            } else if (req_.method() == http::verb::get && target == "/rooms") {
                controller.handleListResource("rooms", *res);
            } else if (req_.method() == http::verb::get && target == "/roles") {
                controller.handleListResource("roles", *res);
            } else if (req_.method() == http::verb::get && target == "/role-permissions") {
                controller.handleListResource("role-permissions", *res);
            } else if (req_.method() == http::verb::get && target == "/sensor-data") {
                controller.handleListResource("sensor-data", *res);
            } else if (req_.method() == http::verb::put && target == "/users") {
                controller.handleUpdateUser(req_, *res);
            } else if (req_.method() == http::verb::delete_ && target == "/users") {
                controller.handleDeleteUser(req_, *res);
            } else if (req_.method() == http::verb::get && target == "/all-topics") {
                controller.handleListResource("all-topics", *res); 
            } else if (req_.method() == http::verb::get && target == "/pvSystems") {
                controller.handleListResource("pvSystems", *res);
            } else {
                res->result(http::status::not_found);
                res->set(http::field::content_type, "application/json");
                res->body() = "{\"error\":\"route not found\"}";
                res->prepare_payload();
            }

            // Antwort an den Client senden
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
    };
};

int main() {
    try {
        auto const address = net::ip::make_address("0.0.0.0");  // Alle Interfaces
        unsigned short port = 8080;

        net::io_context ioc{1};

        // Datenbankverbindung erstellen
        Database db("127.0.0.1", "dashboard_user", "MeinSicheresPasswort123!", "energiedashboard");

        // MQTT-Listener parallel zum HTTP-Server starten
        MqttClient mqttClient(db);
        std::thread mqttThread([&mqttClient]() {
            mqttClient.startListening();
        });

        // Listener für den HTTP-Server starten
        auto listener = std::make_shared<Listener>(ioc, tcp::endpoint{address, port}, db);
        listener->run();

        ioc.run();  // Event Loop starten, um Anfragen zu bearbeiten

        mqttClient.stopListening();
        if (mqttThread.joinable()) {
            mqttThread.join();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
