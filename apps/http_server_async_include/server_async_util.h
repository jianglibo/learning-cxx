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

    // Return a reasonable mime type based on the extension of a file.
    inline beast::string_view
    mime_type(beast::string_view path)
    {
        using beast::iequals;
        auto const ext = [&path]
        {
            auto const pos = path.rfind(".");
            if (pos == beast::string_view::npos)
                return beast::string_view{};
            return path.substr(pos);
        }();
        if (iequals(ext, ".htm"))
            return "text/html";
        if (iequals(ext, ".html"))
            return "text/html";
        if (iequals(ext, ".php"))
            return "text/html";
        if (iequals(ext, ".css"))
            return "text/css";
        if (iequals(ext, ".txt"))
            return "text/plain";
        if (iequals(ext, ".js"))
            return "application/javascript";
        if (iequals(ext, ".json"))
            return "application/json";
        if (iequals(ext, ".xml"))
            return "application/xml";
        if (iequals(ext, ".swf"))
            return "application/x-shockwave-flash";
        if (iequals(ext, ".flv"))
            return "video/x-flv";
        if (iequals(ext, ".png"))
            return "image/png";
        if (iequals(ext, ".jpe"))
            return "image/jpeg";
        if (iequals(ext, ".jpeg"))
            return "image/jpeg";
        if (iequals(ext, ".jpg"))
            return "image/jpeg";
        if (iequals(ext, ".gif"))
            return "image/gif";
        if (iequals(ext, ".bmp"))
            return "image/bmp";
        if (iequals(ext, ".ico"))
            return "image/vnd.microsoft.icon";
        if (iequals(ext, ".tiff"))
            return "image/tiff";
        if (iequals(ext, ".tif"))
            return "image/tiff";
        if (iequals(ext, ".svg"))
            return "image/svg+xml";
        if (iequals(ext, ".svgz"))
            return "image/svg+xml";
        return "application/text";
    }

    // Append an HTTP rel-path to a local filesystem path.
    // The returned path is normalized for the platform.
    std::string
    path_cat(
        beast::string_view base,
        beast::string_view path)
    {
        if (base.empty())
            return std::string(path);
        std::string result(base);
#ifdef BOOST_MSVC
        char constexpr path_separator = '\\';
        if (result.back() == path_separator)
            result.resize(result.size() - 1);
        result.append(path.data(), path.size());
        for (auto &c : result)
            if (c == '/')
                c = path_separator;
#else
        char constexpr path_separator = '/';
        if (result.back() == path_separator)
            result.resize(result.size() - 1);
        result.append(path.data(), path.size());
#endif
        return result;
    }

    // Return a response for the given request.
    //
    // The concrete type of the response message (which depends on the
    // request), is type-erased in message_generator.
    template <class Body, class Allocator>
    http::message_generator
    handle_request(
        beast::string_view doc_root,
        http::request<Body, http::basic_fields<Allocator>> &&req)
    {
        // Returns a bad request response
        auto const bad_request =
            [&req](beast::string_view why)
        {
            http::response<http::string_body> res{http::status::bad_request, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = std::string(why);
            res.prepare_payload();
            return res;
        };

        // Returns a not found response
        auto const not_found =
            [&req](beast::string_view target)
        {
            http::response<http::string_body> res{http::status::not_found, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = "The resource '" + std::string(target) + "' was not found.";
            res.prepare_payload();
            return res;
        };

        // Returns a server error response
        auto const server_error =
            [&req](beast::string_view what)
        {
            http::response<http::string_body> res{http::status::internal_server_error, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = "An error occurred: '" + std::string(what) + "'";
            res.prepare_payload();
            return res;
        };

        // Make sure we can handle the method
        if (req.method() != http::verb::get &&
            req.method() != http::verb::head)
            return bad_request("Unknown HTTP-method");

        // Request path must be absolute and not contain "..".
        if (req.target().empty() ||
            req.target()[0] != '/' ||
            req.target().find("..") != beast::string_view::npos)
            return bad_request("Illegal request-target");

        // Build the path to the requested file
        std::string path = path_cat(doc_root, req.target());
        if (req.target().back() == '/')
            path.append("index.html");

        // Attempt to open the file
        beast::error_code ec;
        http::file_body::value_type body;
        body.open(path.c_str(), beast::file_mode::scan, ec);

        // Handle the case where the file doesn't exist
        if (ec == beast::errc::no_such_file_or_directory)
            return not_found(req.target());

        // Handle an unknown error
        if (ec)
            return server_error(ec.message());

        // Cache the size since we need it after the move
        auto const size = body.size();

        // Respond to HEAD request
        if (req.method() == http::verb::head)
        {
            http::response<http::empty_body> res{http::status::ok, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, mime_type(path));
            res.content_length(size);
            res.keep_alive(req.keep_alive());
            return res;
        }

        // Respond to GET request
        http::response<http::file_body> res{
            std::piecewise_construct,
            std::make_tuple(std::move(body)),
            std::make_tuple(http::status::ok, req.version())};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return res;
    }

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