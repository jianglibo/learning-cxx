#pragma once
#ifndef HANDLER_TASKS_H
#define HANDLER_TASKS_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include "http_handler_util.hpp"
#include "http_handler.hpp"

namespace server_async
{

    template <typename SessionType>
    class TaskRequestHandler : public RequestHandlerbase<SessionType, EmptyBodyRequest>
    {
    public:
        TaskRequestHandler(std::shared_ptr<SessionType> session, const std::string &doc_root, EmptyBodyParser &req0)
            : RequestHandlerbase<SessionType, EmptyBodyRequest>(std::move(session), req0.get()), doc_root(doc_root)
        {
        }

        void handle_request() override
        {
            // Make sure we can handle the method

            if (this->req0.method() != http::verb::get &&
                this->req0.method() != http::verb::head)
                return this->session->queue_write(this->bad_request("Unknown HTTP-method"));

            // Request path must be absolute and not contain "..".
            if (this->req0.target().empty() ||
                this->req0.target()[0] != '/' ||
                this->req0.target().find("..") != beast::string_view::npos)
                return this->session->queue_write(this->bad_request("Illegal request-target"));

            // Build the path to the requested file
            std::string path = path_cat(doc_root, this->req0.target());
            if (this->req0.target().back() == '/')
                path.append("index.html");

            // Attempt to open the file
            beast::error_code ec;
            http::file_body::value_type body;
            body.open(path.c_str(), beast::file_mode::scan, ec);

            // Handle the case where the file doesn't exist
            if (ec == beast::errc::no_such_file_or_directory)
            {
                std::cerr << "File not found: " << doc_root << "/" << path << std::endl;
                return this->session->queue_write(this->not_found(this->req0.target()));
            }

            // Handle an unknown error
            if (ec)
                return this->session->queue_write(this->server_error(ec.message()));

            // Cache the size since we need it after the move
            auto const size = body.size();

            // Respond to HEAD request
            if (this->req0.method() == http::verb::head)
            {
                http::response<http::empty_body> res{http::status::ok, this->req0.version()};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, mime_type(path));
                res.content_length(size);
                res.keep_alive(this->req0.keep_alive());
                return this->session->queue_write(std::move(res));
            }

            // Respond to GET request
            http::response<http::file_body> res{
                std::piecewise_construct,
                std::make_tuple(std::move(body)),
                std::make_tuple(http::status::ok, this->req0.version())};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, mime_type(path));
            res.content_length(size);
            res.keep_alive(this->req0.keep_alive());
            return this->session->queue_write(std::move(res));
        }

    private:
        const std::string &doc_root;
    };

}

#endif