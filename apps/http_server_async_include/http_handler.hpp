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

    template <typename SessionType>
    class FileRequestHandler : public RequestHandlerbase<SessionType, EmptyBodyRequest>
    {
    public:
        FileRequestHandler(std::shared_ptr<SessionType> session, const std::string &doc_root, EmptyBodyParser &req0)
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

    template <typename SessionType>
    class FileUploadHandler : public RequestHandlerbase<SessionType, EmptyBodyRequest>,
                              public std::enable_shared_from_this<FileUploadHandler<SessionType>>
    {
    public:
        FileUploadHandler(std::shared_ptr<SessionType> session, EmptyBodyParser &req0)
            : RequestHandlerbase<SessionType, EmptyBodyRequest>(std::move(session), req0.get()), new_parser(std::move(req0))
        {
        }

        // If this is not a form upload then use a string_body
        // curl -v -F "file=@learn.md" -F "key=value" http://localhost:8080/multipart/form-data
        // curl -v -d "username=myuser&password=mypassword" http://localhost:8080/x-www-form-urlencoded
        // curl -v  -X POST -T learn.md http://localhost:8080/upload/data //100-continue
        // curl -v -X POST -H "Content-Type: application/octet-stream" --data-binary "@learn.md" http://localhost:8080/upload/data
        // ebr[http::field::content_type] == "application/x-www-form-urlencoded" &&
        void handle_request() override
        {
            http::file_body::value_type body;
            beast::error_code ec;
            body.open("upload_tmp_file", beast::file_mode::write, ec);
            if (ec)
            {
                std::cerr << "Error: " << ec.message() << std::endl;
                this->session->do_eof();
                return;
            }
            // http::request<http::file_body> new_req{};
            // std::move(ebr.header());
            //    std::move(body)};
            // https://www.boost.org/doc/libs/1_86_0/libs/beast/doc/html/beast/concepts/Body.html
            // http::request_parser<http::file_body> new_parser{std::move(ep)};
            new_parser.get().body() = std::move(body);
            std::cout << "buffer size: " << this->session->buffer_.size() << std::endl;
            std::cout << "request: " << new_parser.get().base() << std::endl;
            // https://www.boost.org/doc/libs/1_86_0/libs/beast/doc/html/beast/ref/boost__beast__http__async_read/overload1.html
            http::async_read(
                this->session->stream(),
                this->session->buffer_,
                new_parser,
                beast::bind_front_handler(
                    &FileUploadHandler::on_read,
                    this->shared_from_this()));
            // [self = this->shared_from_this(), nsession = this->session->shared_from_this()](beast::error_code ec, std::size_t bytes_transferred)
            // {
            //     boost::ignore_unused(bytes_transferred);
            //     if (ec)
            //     {
            //         std::cerr << "Error: " << ec.message() << std::endl;
            //         return;
            //     }
            //     bool keep_alive = self->new_parser.get().keep_alive();
            //     std::cout << "keep alive: " << keep_alive << std::endl;
            //     http::response<http::empty_body> res{http::status::ok, self->new_parser.get().version()};
            //     nsession->queue_write(std::move(res));
            //     nsession->continue_read_if_needed();
            //     std::cout << "Handling file upload..." << std::endl;
            // });
        }

        void on_read(beast::error_code ec, std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);
            if (ec)
            {
                std::cerr << "Error: " << ec.message() << std::endl;
                return;
            }
            bool keep_alive = new_parser.get()[http::field::connection] == "keep-alive";

            std::cout << "keep alive: " << keep_alive << std::endl;
            http::response<http::empty_body> res{http::status::ok, new_parser.get().version()};
            res.keep_alive(keep_alive);
            this->session->queue_write(std::move(res));
            this->session->continue_read_if_needed();
            std::cout << "Handling file upload..." << std::endl;
        }

    private:
        http::request_parser<http::file_body> new_parser;
    };
}

#endif