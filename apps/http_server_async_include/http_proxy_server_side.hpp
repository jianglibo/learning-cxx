#pragma once
#ifndef HTTP_PROXY_SERVER_SIDE_H
#define HTTP_PROXY_SERVER_SIDE_H

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = asio::ip::tcp;

class ProxyServer
{
public:
    ProxyServer(asio::io_context &ioc, tcp::endpoint endpoint, const std::string &target_host, const std::string &target_port)
        : acceptor_(ioc, endpoint), target_host_(target_host), target_port_(target_port)
    {
        start_accept();
    }

private:
    tcp::acceptor acceptor_;
    std::string target_host_;
    std::string target_port_;

    void start_accept()
    {
        acceptor_.async_accept([this](beast::error_code ec, tcp::socket socket)
                               {
            if (!ec) {
                std::make_shared<Session>(std::move(socket), target_host_, target_port_)->start();
            }
            start_accept(); });
    }

    class Session : public std::enable_shared_from_this<Session>
    {
    public:
        Session(tcp::socket socket, const std::string &target_host, const std::string &target_port)
            : socket_(std::move(socket)), resolver_(socket_.get_executor()), target_stream_(socket_.get_executor()),
              target_host_(target_host), target_port_(target_port) {}

        void start()
        {
            do_read_request();
        }

    private:
        tcp::socket socket_;
        beast::flat_buffer buffer_;
        http::request<http::string_body> req_;
        tcp::resolver resolver_;
        beast::tcp_stream target_stream_;
        std::string target_host_;
        std::string target_port_;
        beast::flat_buffer target_buffer_;
        http::response_parser<http::string_body> res_parser_;

        void do_read_request()
        {
            auto self = shared_from_this();
            http::async_read(socket_, buffer_, req_, [self](beast::error_code ec, std::size_t bytes_transferred)
                             {
                if (!ec) {
                    self->do_resolve();
                } });
        }

        void do_resolve()
        {
            auto self = shared_from_this();
            resolver_.async_resolve(target_host_, target_port_, [self](beast::error_code ec, tcp::resolver::results_type results)
                                    {
                if (!ec) {
                    self->do_connect(results);
                } });
        }

        void do_connect(const tcp::resolver::results_type &results)
        {
            auto self = shared_from_this();
            target_stream_.expires_after(std::chrono::seconds(30));
            target_stream_.async_connect(results, [self](beast::error_code ec, tcp::resolver::results_type::endpoint_type)
                                         {
                if (!ec) {
                    self->do_send_request();
                } });
        }

        void do_send_request()
        {
            auto self = shared_from_this();
            req_.set(http::field::host, target_host_);
            http::async_write(target_stream_, req_, [self](beast::error_code ec, std::size_t bytes_transferred)
                              {
                if (!ec) {
                    self->do_read_response_header();
                } });
        }

        void do_read_response_header()
        {
            auto self = shared_from_this();
            res_parser_.body_limit(1024 * 1024 * 100); // Limit body size to 100MB (adjust as needed)

            http::async_read_header(target_stream_, target_buffer_, res_parser_, [self](beast::error_code ec, std::size_t bytes_transferred)
                                    {
                if (!ec) {
                    self->do_send_response_header();
                } });
        }

        void do_send_response_header()
        {
            auto self = shared_from_this();
            http::response<http::empty_body> res;
            res.result(res_parser_.get().result());
            res.version(res_parser_.get().version());
            res.set(http::field::content_length, res_parser_.content_length().value_or(0));
            res.keep_alive(res_parser_.get().keep_alive());

            http::async_write(socket_, res_parser_.get(), [self](beast::error_code ec, std::size_t)
                              {
                if (!ec) {
                    self->do_stream_body();
                } });
        }

        void do_stream_body()
        {
            auto self = shared_from_this();
            http::async_read_some(target_stream_, target_buffer_, res_parser_, [self](beast::error_code ec, std::size_t bytes_transferred)
                                  {
                if (!ec) {
                    // Forward the chunk to the client
                    self->async_forward_chunk(bytes_transferred);
                } else if (ec == http::error::end_of_stream) {
                    // Clean shutdown when done
                    self->shutdown();
                } });
        }

        void async_forward_chunk(std::size_t bytes_transferred)
        {
            auto self = shared_from_this();

            // Write chunk to client A
            asio::async_write(socket_, target_buffer_.data(), [self, bytes_transferred](beast::error_code ec, std::size_t)
                              {
                if (!ec) {
                    // Clear the consumed chunk and read more if there is more data
                    self->target_buffer_.consume(bytes_transferred);
                    self->do_stream_body();
                } else {
                    self->shutdown();
                } });
        }

        void shutdown()
        {
            beast::error_code ec;
            socket_.shutdown(tcp::socket::shutdown_send, ec);
        }
    };
};

#endif
// int main() {
//     try {
//         asio::io_context ioc;
//         tcp::endpoint endpoint(tcp::v4(), 8080); // Listen on port 8080
//         std::string target_host = "www.example.com"; // Target server C
//         std::string target_port = "80"; // Target server port

//         ProxyServer server(ioc, endpoint, target_host, target_port);
//         ioc.run();
//     }
//     catch (const std::exception& e) {
//         std::cerr << "Exception: " << e.what() << std::endl;
//     }

//     return 0;
// }
