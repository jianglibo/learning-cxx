
#pragma once
#ifndef CONNECTION_SESSION_H
#define CONNECTION_SESSION_H

#include "server_async_util.h"

namespace server_async
{
    //------------------------------------------------------------------------------

    template <class Derived>
    class connection_session
    {
        // Access the derived class, this is part of
        // the Curiously Recurring Template Pattern idiom.
        Derived &
        derived()
        {
            return static_cast<Derived &>(*this);
        }

        // beast::flat_buffer buffer_;

        // Start the asynchronous operation
        template <class Body, class Allocator>
        void do_accept(http::request<Body, http::basic_fields<Allocator>> req)
        {                                           // Connect to the target server specified in the request
            const auto &target_host = req.target(); // Extract the target host
            boost::asio::ip::tcp::resolver resolver(derived().client_socket().get_executor());
            auto endpoints = resolver.resolve(target_host, "443"); // Example: HTTPS port
            tcp::socket target_socket_{derived().client_socket().get_executor()};
            // Initiate an asynchronous connection to the target server
            auto self = derived().shared_from_this();
            tcp::socket &cs = derived().client_socket();

            boost::asio::async_connect(target_socket_, endpoints,
                                       [this, self, &req](beast::error_code ec, const auto & /*endpoint*/)
                                       {
                                           if (!ec)
                                           {
                                               // Send "200 Connection Established" response
                                               http::response<http::string_body> res{http::status::ok, req.version()};
                                               res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                                               res.keep_alive(req.keep_alive());
                                               bool keep_alive = req.keep_alive();

                                               // Serialize the response into a buffer
                                               //    beast::flat_buffer buffer;
                                            //    http::serializer<false, http::empty_body> serializer{res};
                                               //    serializer.prepare_payload(); // Ensure the payload is prepared for serialization
                                               //    beast::flat_buffer response_buffer;
                                               //    beast::ostream(response_buffer) << res;
                                               //    http::write(response_buffer, serializer);

                                            //    beast::http::async_write( // Modify this line to use beast::http::async_write
                                            //        derived().client_socket(),
                                            //        std::move(res),
                                            //        beast::bind_front_handler(
                                            //            &connection_session::on_write,
                                            //            derived().shared_from_this(),
                                            //            keep_alive));
                                           }
                                           else
                                           {
                                               // Handle connection error
                                           }
                                       });

            // Return an empty generator for now, as this is handled asynchronously
            // return http::message_generator();

            // Handle other HTTP methods...

            // derived().ws().set_option(
            //     websocket::stream_base::timeout::suggested(
            //         beast::role_type::server));

            // // Set a decorator to change the Server of the handshake
            // derived().ws().set_option(
            //     websocket::stream_base::decorator(
            //         [](websocket::response_type &res)
            //         {
            //             res.set(http::field::server,
            //                     std::string(BOOST_BEAST_VERSION_STRING) +
            //                         " advanced-server-flex");
            //         }));

            // // Accept the websocket handshake
            // derived().ws().async_accept(
            //     req,
            //     beast::bind_front_handler(
            //         &connection_session::on_accept,
            //         derived().shared_from_this()));
        }

    private:
        void
        on_accept(beast::error_code ec)
        {
            if (ec)
                return fail(ec, "accept");

            // Read a message
            // do_read();
        }

        // void do_read()
        // {
        //     // Read a message into our buffer
        //     derived().ws().async_read(
        //         buffer_,
        //         beast::bind_front_handler(
        //             &connection_session::on_read,
        //             derived().shared_from_this()));
        // }

        // void on_read(
        //     beast::error_code ec,
        //     std::size_t bytes_transferred)
        // {
        //     boost::ignore_unused(bytes_transferred);

        //     if (ec == websocket::error::closed)
        //         return;

        //     if (ec)
        //         return fail(ec, "read");

        //     // Echo the message
        //     derived().ws().text(derived().ws().got_text());
        //     derived().ws().async_write(
        //         // buffer_.data(),
        //         beast::bind_front_handler(
        //             &connection_session::on_write,
        //             derived().shared_from_this()));
        // }

        void on_write(
            beast::error_code ec,
            std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);

            if (ec)
                return fail(ec, "write");

            // Clear the buffer
            // buffer_.consume(buffer_.size());

            // Do another read
            // do_read();
        }

    public:
        // Start the asynchronous operation
        template <class Body, class Allocator>
        void
        run(http::request<Body, http::basic_fields<Allocator>> req)
        {
            do_accept(std::move(req));
        }
        // boost::asio::ip::tcp::socket client_socket_;
        // boost::asio::ip::tcp::socket target_socket_;
        // beast::flat_buffer buffer_; // Buffer for reading/writing data

        // public:
        // Start the relay (bi-directional data transfer)
        // void start()
        // {
        //     do_read_from_client();
        //     do_read_from_target();
        // }

    private:
        // Read from the client and write to the target server
        void do_read_from_client()
        {
            auto self = derived().shared_from_this();
            // client_socket_.async_read_some(buffer_,
            //                                [self](beast::error_code ec, std::size_t bytes_transferred)
            //                                {
            //                                    if (!ec)
            //                                    {
            //                                        // Forward the data to the target server
            //                                        boost::asio::async_write(self->target_socket_, boost::asio::buffer(self->buffer_, bytes_transferred),
            //                                                                 [self](beast::error_code ec, std::size_t)
            //                                                                 {
            //                                                                     if (!ec)
            //                                                                         self->do_read_from_client(); // Continue relaying
            //                                                                 });
            //                                    }
            //                                    else
            //                                    {
            //                                        self->close();
            //                                    }
            //                                });
        }

        // // Read from the target server and write to the client
        // void do_read_from_target()
        // {
        //     auto self = derived().shared_from_this();
        //     target_socket_.async_read_some(buffer_,
        //                                    [self](beast::error_code ec, std::size_t bytes_transferred)
        //                                    {
        //                                        if (!ec)
        //                                        {
        //                                            // Forward the data to the client
        //                                            boost::asio::async_write(self->client_socket_, boost::asio::buffer(self->buffer_, bytes_transferred),
        //                                                                     [self](beast::error_code ec, std::size_t)
        //                                                                     {
        //                                                                         if (!ec)
        //                                                                             self->do_read_from_target(); // Continue relaying
        //                                                                     });
        //                                        }
        //                                        else
        //                                        {
        //                                            self->close();
        //                                        }
        //                                    });
        // }

        // Close both sockets
        // void close()
        // {
        //     boost::system::error_code ec;
        //     // client_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        //     // target_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        // }
    };

    //------------------------------------------------------------------------------

    class plain_connection_session
        : public connection_session<plain_connection_session>,
          public std::enable_shared_from_this<plain_connection_session>
    {
        beast::tcp_stream client_stream_;

    public:
        // Create the session
        explicit plain_connection_session(beast::tcp_stream &&stream) : client_stream_(std::move(stream))
        {
        }

        // Called by the base class
        tcp::socket &client_socket()
        {
            return client_stream_.socket();
        }

        beast::tcp_stream &stream()
        {
            return client_stream_;
        }
    };

    // Handles an SSL WebSocket connection
    class ssl_connection_session
        : public connection_session<ssl_connection_session>,
          public std::enable_shared_from_this<ssl_connection_session>
    {
        ssl::stream<beast::tcp_stream> client_stream_;

    public:
        explicit ssl_connection_session(ssl::stream<beast::tcp_stream> &&stream) : client_stream_(std::move(stream))
        // explicit ssl_connection_session(ssl::stream<beast::tcp_stream> &&stream)
        {
        }

        // Called by the base class
        tcp::socket &client_socket()
        {
            return client_stream_.lowest_layer();
        }

        ssl::stream<beast::tcp_stream> &stream()
        {
            return client_stream_;
        }
    };

    //------------------------------------------------------------------------------

    template <class Body, class Allocator>
    void
    make_connection_session(
        beast::tcp_stream stream,
        http::request<Body, http::basic_fields<Allocator>> req)
    {
        std::make_shared<plain_connection_session>(
            std::move(stream))
            ->run(std::move(req));
    }

    template <class Body, class Allocator>
    void
    make_connection_session(
        ssl::stream<beast::tcp_stream> stream,
        http::request<Body, http::basic_fields<Allocator>> req)
    {
        std::make_shared<ssl_connection_session>(
            std::move(stream))
            ->run(std::move(req));
    }
}
#endif