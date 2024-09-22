#pragma once
#ifndef SERVER_ASYNC_HTTP_HANDLER_H
#define SERVER_ASYNC_HTTP_HANDLER_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include "http_handler_util.hpp"

namespace server_async
{
    

    template <typename RequestType>
    class RequestHandlerBase
    {
    public:
        RequestHandlerBase(RequestType &&req) : req(std::move(req)) {}
        virtual ~RequestHandlerBase() = default;

        // This will be the common interface for handling requests
        virtual http::message_generator handle_request() = 0;

    protected:
        // Utility to create a bad request response
        http::response<http::string_body> bad_request(
            beast::string_view why)
        {
            http::response<http::string_body> res{http::status::bad_request, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = std::string(why);
            res.prepare_payload();
            return res;
        }

        // Utility to create a not found response
        http::response<http::string_body> not_found(
            beast::string_view target)
        {
            http::response<http::string_body> res{http::status::not_found, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = "The resource '" + std::string(target) + "' was not found.";
            res.prepare_payload();
            return res;
        }

        // Utility to create a server error response
        http::response<http::string_body> server_error(
            beast::string_view what)
        {
            http::response<http::string_body> res{http::status::internal_server_error, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = "An error occurred: '" + std::string(what) + "'";
            res.prepare_payload();
            return res;
        }

    protected:
        RequestType req;
    };

    class FileRequestHandler : public RequestHandlerBase<EmptyBodyRequest>
    {
    public:
        FileRequestHandler(const std::string &doc_root, EmptyBodyRequest &&req)
            : RequestHandlerBase<EmptyBodyRequest>(std::move(req)), doc_root(doc_root)
        {
        }

        http::message_generator handle_request() override
        {
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
            if (ec == beast::errc::no_such_file_or_directory){
                std::cerr << "File not found: " << doc_root << "/" <<  path << std::endl;
                return not_found(req.target());
            }

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

    private:
        const std::string &doc_root;
    };

}

#endif