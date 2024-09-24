#pragma once
#ifndef SERVER_ASYNC_HTTP_SESSION_H
#define SERVER_ASYNC_HTTP_SESSION_H

#include "socket_session.hpp"
#include "connection_session.hpp"
#include "socket_copier.h"
#include "http_copier.h"
#include "http_handler.hpp"
#include "http_handler_util.hpp"

namespace server_async
{
    //------------------------------------------------------------------------------

    // Handles an HTTP server connection.
    // This uses the Curiously Recurring Template Pattern so that
    // the same code works with both SSL streams and regular sockets.
    template <class Derived>
    class http_session
    {
        std::shared_ptr<std::string const> doc_root_;
        HandlerEntryPoint<Derived> &handle_func;

        // Access the derived class, this is part of
        // the Curiously Recurring Template Pattern idiom.
        Derived &
        derived()
        {
            return static_cast<Derived &>(*this);
        }

        static constexpr std::size_t queue_limit = 8; // max responses
        std::queue<http::message_generator> response_queue_;

        // The parser is stored in an optional container so we can
        // construct it from scratch it at the beginning of each new message.
        boost::optional<http::request_parser<http::empty_body>> parser_;

    public:
        beast::flat_buffer buffer_;
        // Construct the session
        http_session(
            beast::flat_buffer buffer,
            HandlerEntryPoint<Derived> &handle_func)
            // std::shared_ptr<std::string const> const &doc_root
            // )
            : handle_func(handle_func), buffer_(std::move(buffer))
        {
        }

        void
        do_read()
        {
            // Construct a new parser for each message
            parser_.emplace();

            // Apply a reasonable limit to the allowed size
            // of the body in bytes to prevent abuse.
            // parser_->body_limit(10000);

            // Set the timeout.
            beast::get_lowest_layer(
                derived().stream())
                .expires_after(std::chrono::seconds(30));

            // Read a request using the parser-oriented interface
            // http::async_read(
            //     derived().stream(),
            //     buffer_,
            //     *parser_,
            //     beast::bind_front_handler(
            //         &http_session::on_read,
            //         derived().shared_from_this()));
            http::async_read_header(
                derived().stream(),
                buffer_,
                *parser_,
                beast::bind_front_handler(
                    &http_session::on_read,
                    derived().shared_from_this()));
        }

        void
        on_read(beast::error_code ec, std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);

            // This means they closed the connection
            if (ec == http::error::end_of_stream)
                return derived().do_eof();

            if (ec)
                return fail(ec, "read");

            // See if it is a WebSocket Upgrade
            if (websocket::is_upgrade(parser_->get()))
            {
                // Disable the timeout.
                // The websocket::stream uses its own timeout settings.
                beast::get_lowest_layer(derived().stream()).expires_never();

                // Create a websocket session, transferring ownership
                // of both the socket and the HTTP request.
                return make_websocket_session(
                    derived().release_stream(),
                    parser_->release());
            }
            else if (parser_->get().method() == http::verb::connect)
            {
                // tcp::socket &raw_socket = derived().socket();
                // std::cout << "got target: " << parser_->get().target() << std::endl;
                // assert(parser_->get().target().starts_with("http"));
                // return socket_copier::create(std::move(derived().socket()),
                //                              std::move(buffer_),
                //                              parser_->get().target())
                //     ->start();
                if constexpr (std::is_same_v<Derived, plain_http_session>)
                {
                    std::cout << "Handling plain HTTP session.\n";
                    auto st = derived().release_stream();
                    std::shared_ptr<plain_socket_copy> copier =
                        std::make_shared<plain_socket_copy>(std::move(st), std::move(buffer_), parser_->release());

                    return copier->start();
                }
                else if constexpr (std::is_same_v<Derived, ssl_http_session>)
                {
                    auto st = derived().release_stream();
                    std::shared_ptr<ssl_socket_copy> copier =
                        std::make_shared<ssl_socket_copy>(std::move(st), std::move(buffer_), parser_->release());
                    return copier->start();
                }
                else
                {
                    std::cout << "Unknown HTTP session type.\n";
                    return;
                }
            }
            else if (parser_->get().target().starts_with("http"))
            {
                std::cout << "got absolute target: " << parser_->get().target() << std::endl;
                // start_http_copy(parser_->release());
                // we need send the serialized req first then the unconsumed buffer.
                if constexpr (std::is_same_v<Derived, plain_http_session>)
                {
                    std::cout << "Handling plain HTTP session.\n";
                    auto st = derived().release_stream();
                    std::shared_ptr<plain_http_copy> copier =
                        std::make_shared<plain_http_copy>(std::move(st), parser_->release(), std::move(buffer_));

                    return copier->start();
                }
                else if constexpr (std::is_same_v<Derived, ssl_http_session>)
                {
                    auto st = derived().release_stream();
                    std::shared_ptr<ssl_http_copy> copier =
                        std::make_shared<ssl_http_copy>(std::move(st), parser_->release(), std::move(buffer_));
                    return copier->start();
                }
                else
                {
                    std::cout << "Unknown HTTP session type.\n";
                    return;
                }
            }

            // Send the response
            // queue_write(handle_request(*doc_root_, parser_->release()));
            // assert(handle_func);
            // http::request_parser<http::empty_body> &&v = std::move(parser_.get());
            // http::request_parser<http::empty_body> v = parser_.get();
            // http::request_parser<http::empty_body> r1{};
            // http::request_parser<http::empty_body> r2 = r1;
            // http::request_parser<http::empty_body> r2{r1};
            // http::request_parser<http::empty_body> r2{std::move(r1)};
            // http::request_parser<http::string_body> r2{std::move(r1)};

            auto const &headers = parser_->get().base();

            handle_func(derived().shared_from_this(), std::move(parser_.get()));
            // queue_write(std::make_shared<FileRequestHandler>(*doc_root_, parser_->release())->handle_request());

            // If we aren't at the queue limit, try to pipeline another request
            // if handle_func take long time to call queue_write, the queue may exceed the limit. but it's no harm.
            // if (response_queue_.size() < queue_limit)
            //     do_read();
        }

        void continue_read_if_needed()
        {
            if (response_queue_.size() < queue_limit)
                do_read();
        }

        void
        queue_write(http::message_generator response)
        {
            // Allocate and store the work
            response_queue_.push(std::move(response));
            // // move from on_read to here. because the handle_func may access the underlying buffer.
            // if (response_queue_.size() < queue_limit)
            //     do_read();

            // If there was no previous work, start the write loop
            if (response_queue_.size() == 1)
                do_write();
        }

        // Called to start/continue the write-loop. Should not be called when
        // write_loop is already active.
        void
        do_write()
        {
            if (!response_queue_.empty())
            {
                bool keep_alive = response_queue_.front().keep_alive();

                beast::async_write(
                    derived().stream(),
                    std::move(response_queue_.front()),
                    beast::bind_front_handler(
                        &http_session::on_write,
                        derived().shared_from_this(),
                        keep_alive));
            }
        }

        void
        on_write(
            bool keep_alive,
            beast::error_code ec,
            std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);

            if (ec)
                return fail(ec, "write");

            if (!keep_alive)
            {
                // This means we should close the connection, usually because
                // the response indicated the "Connection: close" semantic.
                return derived().do_eof();
            }

            // Resume the read if it has been paused
            // if (response_queue_.size() == queue_limit)
            if (response_queue_.size() >= queue_limit)
                do_read();

            response_queue_.pop();

            do_write();
        }
        // virtual boost::asio::io_context &get_io_context() = 0;
    };

    //------------------------------------------------------------------------------
    // Handles a plain HTTP connection
    class plain_http_session
        : public http_session<plain_http_session>,
          public std::enable_shared_from_this<plain_http_session>
    {
        beast::tcp_stream stream_;

    public:
        // Create the session
        plain_http_session(
            beast::tcp_stream &&stream,
            beast::flat_buffer &&buffer,
            HandlerEntryPoint<plain_http_session> &handle_func)
            : http_session<plain_http_session>(
                  std::move(buffer), handle_func),
              stream_(std::move(stream))
        {
        }

        // Start the session
        void
        run()
        {
            this->do_read();
        }

        // Called by the base class
        beast::tcp_stream &
        stream()
        {
            return stream_;
        }
        tcp::socket &
        socket()
        {
            return stream_.socket();
        }

        // boost::asio::io_context &get_io_context()
        // {
        //     // return boost::asio::use_service<boost::asio::io_context>(stream_.get_executor().context());
        //     tcp::resolver resolver_{stream_.get_executor().context()};
        //     return socket().get_executor().context();
        // }

        // Called by the base class
        beast::tcp_stream
        release_stream()
        {
            return std::move(stream_);
        }

        // Called by the base class
        void
        do_eof()
        {
            // Send a TCP shutdown
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

            // At this point the connection is closed gracefully
        }
    };

    //------------------------------------------------------------------------------

    // Handles an SSL HTTP connection
    class ssl_http_session
        : public http_session<ssl_http_session>,
          public std::enable_shared_from_this<ssl_http_session>
    {
        ssl::stream<beast::tcp_stream> stream_;

    public:
        // Create the http_session
        ssl_http_session(
            beast::tcp_stream &&stream,
            ssl::context &ctx,
            beast::flat_buffer &&buffer,
            HandlerEntryPoint<ssl_http_session> &handle_func)
            // std::shared_ptr<std::string const> const &doc_root
            // )
            : http_session<ssl_http_session>(
                  std::move(buffer),
                  handle_func
                  //   doc_root
                  ),
              stream_(std::move(stream), ctx)
        {
        }

        // Start the session
        void
        run()
        {
            // Set the timeout.
            beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

            // Perform the SSL handshake
            // Note, this is the buffered version of the handshake.
            stream_.async_handshake(
                ssl::stream_base::server,
                buffer_.data(),
                beast::bind_front_handler(
                    &ssl_http_session::on_handshake,
                    shared_from_this()));
        }

        // Called by the base class
        ssl::stream<beast::tcp_stream> &
        stream()
        {
            return stream_;
        }

        tcp::socket &
        socket()
        {
            return stream_.lowest_layer();
        }
        // boost::asio::io_context &get_io_context()
        // {
        //     // return boost::asio::use_service<boost::asio::io_context>(stream_.get_executor().context());
        //     return stream_.get_executor().context();
        // }

        // Called by the base class
        ssl::stream<beast::tcp_stream>
        release_stream()
        {
            return std::move(stream_);
        }

        // Called by the base class
        void
        do_eof()
        {
            // Set the timeout.
            beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

            // Perform the SSL shutdown
            stream_.async_shutdown(
                beast::bind_front_handler(
                    &ssl_http_session::on_shutdown,
                    shared_from_this()));
        }

    private:
        void
        on_handshake(
            beast::error_code ec,
            std::size_t bytes_used)
        {
            if (ec)
                return fail(ec, "handshake");

            // Consume the portion of the buffer used by the handshake
            buffer_.consume(bytes_used);

            do_read();
        }

        void
        on_shutdown(beast::error_code ec)
        {
            if (ec)
                return fail(ec, "shutdown");

            // At this point the connection is closed gracefully
        }
    };
}
#endif