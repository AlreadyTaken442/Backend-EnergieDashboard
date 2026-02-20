#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <iostream>
#include <memory>
#include <string>

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>
using tcp = net::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// This function produces an HTTP response for the given request.
http::response<http::string_body> handle_request(http::request<http::string_body> const& req) {
    // Respond to GET request with "Hello, World!"
    if (req.method() == http::verb::get) {
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "Beast");
        res.set(http::field::content_type, "text/plain");
        res.keep_alive(req.keep_alive());
        res.body() = "Hello, World!";
        res.prepare_payload();
        return res;
    }

    // Default response for unsupported methods
    http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
    res.set(http::field::server, "Beast");
    res.keep_alive(req.keep_alive());
    res.prepare_payload(); // Content-Length: 0 for an empty body
    return res;
}

// This class handles an HTTP server connection.
class Session : public std::enable_shared_from_this<Session> {
    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;

public:
    explicit Session(tcp::socket socket) : socket_(std::move(socket)) {}

    void run() {
        do_read();
    }

private:
    void do_read() {
        auto self(shared_from_this());
        http::async_read(socket_, buffer_, req_, [this, self](beast::error_code ec, std::size_t) {
            if (ec == http::error::end_of_stream) {
                beast::error_code ignored;
                socket_.shutdown(tcp::socket::shutdown_send, ignored);
                return;
            }
            if (!ec) {
                do_write(handle_request(req_));
            }
        });
    }

    void do_write(http::response<http::string_body> res) {
        auto self(shared_from_this());
        auto sp = std::make_shared<http::response<http::string_body>>(std::move(res));

        http::async_write(socket_, *sp, [this, self, sp](beast::error_code ec, std::size_t) {
            if (ec) {
                return;
            }

            // The response may be marked `keep-alive`, so do not always close the socket.
            if (req_.keep_alive()) {
                req_ = {};
                buffer_.consume(buffer_.size());
                do_read();
                return;
            }

            beast::error_code ignored;
            socket_.shutdown(tcp::socket::shutdown_send, ignored);
        });
    }
};

// This class accepts incoming connections and launches the sessions.
class Listener : public std::enable_shared_from_this<Listener> {
    net::io_context& ioc_;
    tcp::acceptor acceptor_;

public:
    Listener(net::io_context& ioc, tcp::endpoint endpoint)
        : ioc_(ioc), acceptor_(net::make_strand(ioc)) {
        beast::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            std::cerr << "Open error: " << ec.message() << std::endl;
            return;
        }

        // Allow address reuse
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if (ec) {
            std::cerr << "Set option error: " << ec.message() << std::endl;
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if (ec) {
            std::cerr << "Bind error: " << ec.message() << std::endl;
            return;
        }

        // Start listening for connections
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
            [self](beast::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<Session>(std::move(socket))->run();
                }
                self->do_accept();
            });
    }
};

int main() {
    try {
        auto const address = net::ip::make_address("0.0.0.0"); // If this doesn't compile, add: #include <boost/asio/ip/address.hpp>
        unsigned short port = 8080;

        net::io_context ioc{1};

        std::make_shared<Listener>(ioc, tcp::endpoint{address, port})->run();

        ioc.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}