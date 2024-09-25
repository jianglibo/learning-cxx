#pragma once
#ifndef SERVER_ASYNC_HTTP_HANDLER_H
#define SERVER_ASYNC_HTTP_HANDLER_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include "http_handler_util.hpp"

namespace server_async
{

    template <typename SessionType, typename RequestType>
    class RequestHandlerbase
    {
    public:
        RequestHandlerbase(std::shared_ptr<SessionType> &&session, RequestType req0 /*copy*/) : session(std::move(session)), req0(req0) {}
        virtual ~RequestHandlerbase() = default;
        // This will be the common interface for handling requests
        virtual void handle_request() = 0;

    protected:
        // Utility to create a bad request response
        http::response<http::string_body> bad_request(
            beast::string_view why)
        {
            http::response<http::string_body> res{http::status::bad_request, req0.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req0.keep_alive());
            res.body() = std::string(why);
            res.prepare_payload();
            return res;
        }

        // Utility to create a not found response
        http::response<http::string_body> not_found(
            beast::string_view target)
        {
            http::response<http::string_body> res{http::status::not_found, req0.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req0.keep_alive());
            res.body() = "The resource '" + std::string(target) + "' was not found.";
            res.prepare_payload();
            return res;
        }

        // Utility to create a server error response
        http::response<http::string_body> server_error(
            beast::string_view what)
        {
            http::response<http::string_body> res{http::status::internal_server_error, req0.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req0.keep_alive());
            res.body() = "An error occurred: '" + std::string(what) + "'";
            res.prepare_payload();
            return res;
        }
        RequestType req0;
        std::shared_ptr<SessionType> session;
    };
}

#endif