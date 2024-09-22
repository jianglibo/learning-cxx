#pragma once

#ifndef SERVER_ASYNC_UTIL_H
#define SERVER_ASYNC_UTIL_H

#include "server_certificate.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/make_unique.hpp>
#include <boost/optional.hpp>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <vector>
#include <fstream>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace server_async
{

    //------------------------------------------------------------------------------

    // Report a failure
    inline void
    fail(beast::error_code ec, char const *what)
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

        if (ec == net::ssl::error::stream_truncated)
            return;

        std::cerr << what << ": " << ec.message() << "\n";
    }

    // void serialize_headers_with_visitor(beast::flat_buffer &buffer, const http::request<http::empty_body> &req)
    // {
    //     // Create a serializer for the request
    //     http::request_serializer<http::empty_body> sr{req};

    //     // Set split mode to true (this ensures only headers are serialized)
    //     sr.split(true);

    //     boost::system::error_code ec;

    //     // Visitor lambda that copies the serialized data to the buffer
    //     auto visitor = [&buffer](boost::system::error_code &ec, auto const &const_buffers)
    //     {
    //         // If there's an error, exit
    //         if (ec)
    //         {
    //             std::cerr << "Visitor error: " << ec.message() << std::endl;
    //             return;
    //         }

    //         // Copy serialized data into the flat_buffer
    //         buffer.commit(boost::asio::buffer_copy(buffer.prepare(boost::asio::buffer_size(const_buffers)), const_buffers));
    //     };

    //     // Serialize headers incrementally using the visitor
    //     while (!sr.is_done())
    //     {
    //         std::cout << "is header done: " << sr.is_header_done() << std::endl;
    //         sr.next(ec, visitor);
    //         if (ec)
    //         {
    //             std::cerr << "Error during serialization: " << ec.message() << std::endl;
    //             break;
    //         }
    //     }
    // }

    // The detail namespace means "not public"
    namespace detail
    {

        // This helper is needed for C++11.
        // When invoked with a buffer sequence, writes the buffers `to the std::ostream`.
        template <class Serializer>
        class write_ostream_helper
        {
            Serializer &sr_;
            std::ostream &os_;

        public:
            write_ostream_helper(Serializer &sr, std::ostream &os)
                : sr_(sr), os_(os)
            {
            }

            // This function is called by the serializer
            template <class ConstBufferSequence>
            void
            operator()(std::error_code &ec, ConstBufferSequence const &buffers) const
            {
                // Error codes must be cleared on success
                ec = {};

                // Keep a running total of how much we wrote
                std::size_t bytes_transferred = 0;

                // Loop over the buffer sequence
                for (auto it = boost::asio::buffer_sequence_begin(buffers);
                     it != boost::asio::buffer_sequence_end(buffers); ++it)
                {
                    // This is the next buffer in the sequence
                    boost::asio::const_buffer const buffer = *it;

                    // Write it to the std::ostream
                    os_.write(
                        reinterpret_cast<char const *>(buffer.data()),
                        buffer.size());

                    // If the std::ostream fails, convert it to an error code
                    if (os_.fail())
                    {
                        // ec = boost::beast::errc::io_error;
                        ec = std::make_error_code(std::errc::io_error);
                        return;
                    }

                    // Adjust our running total
                    bytes_transferred += buffer_size(buffer);
                }

                // Inform the serializer of the amount we consumed
                sr_.consume(bytes_transferred);
            }
        };
    } // namespace detail

    /** Write a message to a `std::ostream`.

This function writes the serialized representation of the
HTTP/1 message to the sream.

@param os The `std::ostream` to write to.

@param msg The message to serialize.

@param ec Set to the error, if any occurred.
*/
    template <
        bool isRequest,
        class Body,
        class Fields>
    void
    write_ostream(
        std::ostream &os,
        http::message<isRequest, Body, Fields> &msg,
        beast::error_code &ec)
    {
        // Create the serializer instance
        http::serializer<isRequest, Body, Fields> sr{msg};

        // This lambda is used as the "visit" function
        detail::write_ostream_helper<decltype(sr)> lambda{sr, os};
        do
        {
            // In C++14 we could use a generic lambda but since we want
            // to require only C++11, the lambda is written out by hand.
            // This function call retrieves the next serialized buffers.
            sr.next(ec, lambda);
            if (ec)
                return;
        } while (!sr.is_done());
    }

    std::string read_whole_file(const std::string &filename)
    {
        std::ifstream file(filename);
        if (!file)
        {
            throw std::runtime_error("Could not open the file!");
        }
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return content;
    }

}
#endif