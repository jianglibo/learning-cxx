#pragma once
#ifndef HTTP_CLIENT_ASYNC_HPP
#define HTTP_CLIENT_ASYNC_HPP

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ssl.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <future>

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl; // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

namespace client_async
{
    //------------------------------------------------------------------------------

    // Report a failure
    void fail(beast::error_code ec, char const *what)
    {
        std::cerr << what << ": " << ec.message() << "\n";
    }

    // Performs an HTTP GET and prints the response
    class session : public std::enable_shared_from_this<session>
    {
        tcp::resolver resolver_;
        beast::tcp_stream stream_;
        beast::flat_buffer buffer_; // (Must persist between reads)
        http::request<http::empty_body> req_;
        http::response<http::string_body> res_;
        std::promise<http::response<http::string_body>> response_promise_;
        std::future<http::response<http::string_body>> response_future_;

    public:
        // Objects are constructed with a strand to
        // ensure that handlers do not execute concurrently.
        explicit session(net::io_context &ioc)
            : resolver_(net::make_strand(ioc)), stream_(net::make_strand(ioc)), response_future_(response_promise_.get_future())
        {
        }

        std::future<http::response<http::string_body>> &get_response_future()
        {
            return response_future_;
        }

        // Start the asynchronous operation
        void
        run(
            char const *host,
            char const *port,
            char const *target,
            int version)
        {
            // Set up an HTTP GET request message
            req_.version(version);
            req_.method(http::verb::get);
            req_.target(target);
            req_.set(http::field::host, host);
            req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // Look up the domain name
            resolver_.async_resolve(
                host,
                port,
                beast::bind_front_handler(
                    &session::on_resolve,
                    shared_from_this()));
        }

        void
        on_resolve(
            beast::error_code ec,
            tcp::resolver::results_type results)
        {
            if (ec)
                return fail(ec, "resolve");

            // Set a timeout on the operation
            stream_.expires_after(std::chrono::seconds(30));

            // Make the connection on the IP address we get from a lookup
            stream_.async_connect(
                results,
                beast::bind_front_handler(
                    &session::on_connect,
                    shared_from_this()));
        }

        void
        on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
        {
            if (ec)
                return fail(ec, "connect");

            // Set a timeout on the operation
            stream_.expires_after(std::chrono::seconds(30));

            // Send the HTTP request to the remote host
            http::async_write(stream_, req_,
                              beast::bind_front_handler(
                                  &session::on_write,
                                  shared_from_this()));
        }

        void
        on_write(
            beast::error_code ec,
            std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);

            if (ec)
                return fail(ec, "write");

            // Receive the HTTP response
            http::async_read(stream_, buffer_, res_,
                             beast::bind_front_handler(
                                 &session::on_read,
                                 shared_from_this()));
        }

        void
        on_read(
            beast::error_code ec,
            std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);

            if (ec)
            {
                response_promise_.set_exception(std::make_exception_ptr(std::runtime_error(ec.message())));
                return fail(ec, "read");
            }

            // Set the response to the promise
            response_promise_.set_value(res_);

            // Write the message to standard out
            // std::cout << res_ << std::endl;

            // Gracefully close the socket
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // not_connected happens sometimes so don't bother reporting it.
            if (ec && ec != beast::errc::not_connected)
                return fail(ec, "shutdown");

            // If we get here then the connection is closed gracefully
        }
    };

    // Performs an HTTP GET and prints the response
    class session_ssl : public std::enable_shared_from_this<session_ssl>
    {
        tcp::resolver resolver_;
        ssl::stream<beast::tcp_stream> stream_;
        beast::flat_buffer buffer_; // (Must persist between reads)
        http::request<http::empty_body> req_;
        http::response<http::string_body> res_;
        std::promise<http::response<http::string_body>> response_promise_;
        std::future<http::response<http::string_body>> response_future_;

    public:
        explicit session_ssl(
            net::any_io_executor ex,
            ssl::context &ctx)
            : resolver_(ex), stream_(ex, ctx), response_future_(response_promise_.get_future())
        {
        }

        std::future<http::response<http::string_body>> &get_response_future()
        {
            return response_future_;
        }
        // Start the asynchronous operation
        void
        run(
            char const *host,
            char const *port,
            char const *target,
            int version)
        {
            // Set SNI Hostname (many hosts need this to handshake successfully)
            if (!SSL_set_tlsext_host_name(stream_.native_handle(), host))
            {
                beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
                std::cerr << ec.message() << "\n";
                return;
            }

            // Set up an HTTP GET request message
            req_.version(version);
            req_.method(http::verb::get);
            req_.target(target);
            req_.set(http::field::host, host);
            req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // Look up the domain name
            resolver_.async_resolve(
                host,
                port,
                beast::bind_front_handler(
                    &session_ssl::on_resolve,
                    shared_from_this()));
        }

        void
        on_resolve(
            beast::error_code ec,
            tcp::resolver::results_type results)
        {
            if (ec)
                return fail(ec, "resolve");

            // Set a timeout on the operation
            beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

            // Make the connection on the IP address we get from a lookup
            beast::get_lowest_layer(stream_).async_connect(
                results,
                beast::bind_front_handler(
                    &session_ssl::on_connect,
                    shared_from_this()));
        }

        void
        on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
        {
            if (ec)
                return fail(ec, "connect");

            // Perform the SSL handshake
            stream_.async_handshake(
                ssl::stream_base::client,
                beast::bind_front_handler(
                    &session_ssl::on_handshake,
                    shared_from_this()));
        }

        void
        on_handshake(beast::error_code ec)
        {
            if (ec)
                return fail(ec, "handshake");

            // Set a timeout on the operation
            beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

            // Send the HTTP request to the remote host
            http::async_write(stream_, req_,
                              beast::bind_front_handler(
                                  &session_ssl::on_write,
                                  shared_from_this()));
        }

        void
        on_write(
            beast::error_code ec,
            std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);

            if (ec)
                return fail(ec, "write");

            // Receive the HTTP response
            http::async_read(stream_, buffer_, res_,
                             beast::bind_front_handler(
                                 &session_ssl::on_read,
                                 shared_from_this()));
        }

        void
        on_read(
            beast::error_code ec,
            std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);

            if (ec)
            {
                response_promise_.set_exception(std::make_exception_ptr(std::runtime_error(ec.message())));
                return fail(ec, "read");
            }

            // Write the message to standard out
            // Set the response to the promise
            response_promise_.set_value(res_);

            // Set a timeout on the operation
            beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

            // Gracefully close the stream
            stream_.async_shutdown(
                beast::bind_front_handler(
                    &session_ssl::on_shutdown,
                    shared_from_this()));
        }

        void
        on_shutdown(beast::error_code ec)
        {
            // ssl::error::stream_truncated, also known as an SSL "short read",
            // indicates the peer closed the connection without performing the
            // required closing handshake (for example, Google does this to
            // improve performance). Generally this can be a security issue,
            // but if your communication protocol is self-terminated (as
            // it is with both HTTP and WebSocket) then you may simply
            // ignore the lack of close_notify.
            //
            // https://github.com/boostorg/beast/issues/38
            //
            // https://security.stackexchange.com/questions/91435/how-to-handle-a-malicious-ssl-tls-shutdown
            //
            // When a short read would cut off the end of an HTTP message,
            // Beast returns the error beast::http::error::partial_message.
            // Therefore, if we see a short read here, it has occurred
            // after the message has been completed, so it is safe to ignore it.

            if (ec != net::ssl::error::stream_truncated)
                return fail(ec, "shutdown");
        }
    };

    class ClientPoolSsl
    {
    private:
        // The io_context is required for all I/O
        net::io_context ioc;
        // The SSL context is required, and holds certificates
        ssl::context ctx;
        int threads;
        std::vector<std::thread> thread_pool;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard;

    public:
        ClientPoolSsl(int threads) : threads(threads),
                                     ioc{threads},
                                     ctx{ssl::context::tlsv12},
                                     work_guard(boost::asio::make_work_guard(ioc))
        {
        }

        void start(bool blocking = true)
        {
            ctx.set_default_verify_paths();
            // Verify the remote server's certificate
            ctx.set_verify_mode(boost::asio::ssl::verify_peer);

            for (std::size_t i = 0; i < threads; ++i)
            {
                thread_pool.emplace_back([this]
                                         {
                                             std::cout << "ico.run() called." << std::endl;
                                             ioc.run(); // Each thread runs the io_context
                                         });
            }
            // Wait for all threads to finish
            for (auto &t : thread_pool)
            {
                if (blocking)
                {
                    if (t.joinable())
                    {
                        t.join();
                    }
                }
                else
                {
                    t.detach();
                }
            }
        }

        void stop()
        {
            // boost::asio::post(ioc, [this]
            //                   {
            ioc.stop(); // Stop the io_context
                        //   });
            // for (auto &t : thread_pool)
            // {
            //     if (t.joinable())
            //     {
            //         std::cout << "joining" << std::endl;
            //         t.join();
            //     }
            // }
        }

        std::shared_ptr<session> createSession()
        {
            return std::make_shared<session>(ioc);
        }
        std::shared_ptr<session_ssl> createSessionSsl()
        {
            return std::make_shared<session_ssl>(ioc.get_executor(), ctx);
        }
    };
}

#endif

//------------------------------------------------------------------------------
// int main(int argc, char **argv)
// {
//     // Check command line arguments.
//     if (argc != 4 && argc != 5)
//     {
//         std::cerr << "Usage: http-client-async <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n"
//                   << "Example:\n"
//                   << "    http-client-async www.example.com 80 /\n"
//                   << "    http-client-async www.example.com 80 / 1.0\n";
//         return EXIT_FAILURE;
//     }
//     auto const host = argv[1];
//     auto const port = argv[2];
//     auto const target = argv[3];
//     int version = argc == 5 && !std::strcmp("1.0", argv[4]) ? 10 : 11;

//     // The io_context is required for all I/O
//     net::io_context ioc;

//     // Launch the asynchronous operation
//     std::make_shared<session>(ioc)->run(host, port, target, version);

//     // Run the I/O service. The call will return when
//     // the get operation is complete.
//     ioc.run();

//     return EXIT_SUCCESS;
// }