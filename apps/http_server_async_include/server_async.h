#pragma once

#ifndef SERVER_ASYNC_H
#define SERVER_ASYNC_H

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <filesystem>

#include "server_async_util.h"
#include "http_session.hpp"
#include "http_handler_util.hpp"


namespace server_async
{

    //------------------------------------------------------------------------------

    // Detects SSL handshakes
    class detect_session : public std::enable_shared_from_this<detect_session>
    {
        beast::tcp_stream stream_;
        ssl::context &ctx_;
        std::shared_ptr<std::string const> doc_root_;
        beast::flat_buffer buffer_;
        HandlerEntryPoint<plain_http_session> &plain_handle_func;
        HandlerEntryPoint<ssl_http_session> &ssl_handle_func;

    public:
        explicit detect_session(
            tcp::socket &&socket,
            ssl::context &ctx,
            std::shared_ptr<std::string const> const &doc_root,
            HandlerEntryPoint<plain_http_session> &plain_handle_func,
            HandlerEntryPoint<ssl_http_session> &ssl_handle_func)
            : stream_(std::move(socket)), ctx_(ctx), doc_root_(doc_root), plain_handle_func(plain_handle_func), ssl_handle_func(ssl_handle_func)
        {
        }

        // Launch the detector
        void
        run()
        {
            // We need to be executing within a strand to perform async operations
            // on the I/O objects in this session. Although not strictly necessary
            // for single-threaded contexts, this example code is written to be
            // thread-safe by default.
            net::dispatch(
                stream_.get_executor(),
                beast::bind_front_handler(
                    &detect_session::on_run,
                    this->shared_from_this()));
        }

        void
        on_run()
        {
            // Set the timeout.
            stream_.expires_after(std::chrono::seconds(30));

            beast::async_detect_ssl(
                stream_,
                buffer_,
                beast::bind_front_handler(
                    &detect_session::on_detect,
                    this->shared_from_this()));
        }

        void
        on_detect(beast::error_code ec, bool result)
        {
            if (ec)
                return fail(ec, "detect");

            if (result)
            {
                // Launch SSL session
                std::make_shared<ssl_http_session>(
                    std::move(stream_),
                    ctx_,
                    std::move(buffer_),
                    ssl_handle_func)
                    ->run();
                return;
            }

            // Launch plain session
            std::make_shared<plain_http_session>(
                std::move(stream_),
                std::move(buffer_),
                plain_handle_func)
                ->run();
        }
    };

    // Accepts incoming connections and launches the sessions
    class listener : public std::enable_shared_from_this<listener>
    {
        net::io_context &ioc_;
        ssl::context &ctx_;
        tcp::acceptor acceptor_;
        std::shared_ptr<std::string const> doc_root_;
        HandlerEntryPoint<plain_http_session> &plain_handle_func;
        HandlerEntryPoint<ssl_http_session> &ssl_handle_func;

    public:
        listener(
            net::io_context &ioc,
            ssl::context &ctx,
            tcp::endpoint endpoint,
            std::shared_ptr<std::string const> const &doc_root,
            HandlerEntryPoint<plain_http_session> &plain_handle_func,
            HandlerEntryPoint<ssl_http_session> &ssl_handle_func)
            : ioc_(ioc), ctx_(ctx), acceptor_(net::make_strand(ioc)), doc_root_(doc_root), plain_handle_func(plain_handle_func), ssl_handle_func(ssl_handle_func)
        {
            beast::error_code ec;

            // Open the acceptor
            acceptor_.open(endpoint.protocol(), ec);
            if (ec)
            {
                fail(ec, "open");
                return;
            }

            // Allow address reuse
            acceptor_.set_option(net::socket_base::reuse_address(true), ec);
            if (ec)
            {
                fail(ec, "set_option");
                return;
            }

            // Bind to the server address
            acceptor_.bind(endpoint, ec);
            if (ec)
            {
                fail(ec, "bind");
                return;
            }

            // Start listening for connections
            acceptor_.listen(
                net::socket_base::max_listen_connections, ec);
            if (ec)
            {
                fail(ec, "listen");
                return;
            }
        }

        // Start accepting incoming connections
        void
        run()
        {
            do_accept();
        }

    private:
        void
        do_accept()
        {
            // The new connection gets its own strand
            acceptor_.async_accept(
                net::make_strand(ioc_),
                beast::bind_front_handler(
                    &listener::on_accept,
                    shared_from_this()));
        }

        void
        on_accept(beast::error_code ec, tcp::socket socket)
        {
            if (ec)
            {
                fail(ec, "accept");
            }
            else
            {
                // Create the detector http_session and run it
                std::make_shared<detect_session>(
                    std::move(socket),
                    ctx_,
                    doc_root_,
                    plain_handle_func,
                    ssl_handle_func)
                    ->run();
            }

            // Accept another connection
            do_accept();
        }
    };

    class HttpServer
    {
    private:
        // The io_context is required for all I/O
        net::io_context ioc;
        // The SSL context is required, and holds certificates
        ssl::context ctx;
        std::shared_ptr<std::string const> const doc_root;
        unsigned short const port;
        net::ip::address address;
        int threads;
        std::vector<std::thread> v;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard;

    public:
        HttpServer(net::ip::address address,
                   unsigned short port,
                   std::shared_ptr<std::string const> const doc_root,
                   int threads) : address(address),
                                  doc_root(doc_root),
                                  port(port),
                                  threads(threads),
                                  ioc{threads},
                                  ctx{ssl::context::tlsv12},
                                  work_guard(boost::asio::make_work_guard(ioc))
        {
        }

        void start(SSLCertHolder ssl_cert_holder,
                   HandlerEntryPoint<plain_http_session> &plain_handler,
                   HandlerEntryPoint<ssl_http_session> &ssl_handler)
        {
            // HandlerFunc<plain_http_session> plain_handler_func = plain_handler;
            // This holds the self-signed certificate used by the server
            load_server_certificate(ctx, ssl_cert_holder.cert, ssl_cert_holder.key, ssl_cert_holder.dh);
#ifdef _WIN32
            DWORD pid = GetCurrentProcessId(); // Get PID on Windows
            std::cout << "Process ID (Windows): " << pid << std::endl;
#else
            pid_t pid = getpid(); // Get PID on Linux
            std::cout << "Process ID (Linux): " << pid << std::endl;
#endif
            std::filesystem::path cwd = std::filesystem::current_path();
            std::cout << "Current working directory: " << cwd << std::endl;

            // Create and launch a listening port
            std::make_shared<listener>(
                ioc,
                ctx,
                tcp::endpoint{address, port},
                doc_root,
                plain_handler,
                ssl_handler)
                ->run();

            // Capture SIGINT and SIGTERM to perform a clean shutdown
            net::signal_set signals(ioc, SIGINT, SIGTERM);
            signals.async_wait(
                [&](beast::error_code const &, int)
                {
                    // Stop the `io_context`. This will cause `run()`
                    // to return immediately, eventually destroying the
                    // `io_context` and all of the sockets in it.
                    ioc.stop();
                });

            // Run the I/O service on the requested number of threads
            v.reserve(threads - 1);
            for (auto i = threads - 1; i > 0; --i)
                v.emplace_back(
                    [this]
                    {
                        ioc.run();
                    });
            ioc.run();

            // (If we get here, it means we got a SIGINT or SIGTERM)

            // Block until all the threads exit
            for (auto &t : v)
                t.join();
        }

        void stop()
        {
            ioc.stop();
            for (auto &thread : v) // Assuming you have a vector of threads
            {
                thread.join(); // Wait for each thread to finish
            }
        }
    };
}
#endif