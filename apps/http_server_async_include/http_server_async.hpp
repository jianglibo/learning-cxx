#pragma once
#ifndef HTTP_SERVER_ASYNC__H
#define HTTP_SERVER_ASYNC__H

#include "http_handler_util.hpp"
#include "string_util.hpp"
#include "handler_file.hpp"

namespace server_async
{
    template <typename SessionType>
    struct handler : server_async::HandlerEntryPoint<SessionType>
    {
        void operator()(std::shared_ptr<SessionType> session, server_async::EmptyBodyParser &&ep)
        {
            server_async::EmptyBodyRequest &ebr = ep.get();

            boost::system::result<boost::urls::url> rv = boost::urls::parse_origin_form(ebr.target());
            if (rv.has_error())
            {
                std::cerr << "Error: " << rv.error().message() << std::endl;
                return session->do_eof();
            }

            const std::string &path = rv.value().path();
            size_t pos = 0;
            std::vector<std::string_view> segments = server_async::all_segments(path);
            // start router
            if (segments.empty())
            {
                std::cout << "root path" << std::endl;
            }
            else if (segments[0] == "tasks")
            {
                switch (segments.size())
                {
                case 1: // /tasks
                    /* code */
                    break;
                case 2: // tasks/t_uuid
                    /* code */
                    break;
                case 3: // tasks/t_uuid/...
                case 4:
                    /* code */
                    break;

                default:
                    break;
                }
            }
            else if (path == "/index.html")
            {
                std::cout << "index.html" << std::endl;
            }
            else if (path == "/favicon.ico")
            {
                std::cout << "favicon.ico" << std::endl;
            }
            else
            {
                std::cout << "unknown path" << std::endl;
            }

            if (ebr[http::field::content_type].find("multipart/form-data") != std::string::npos ||
                ebr.target() == "/upload/data")
            {
                std::make_shared<server_async::FileUploadHandler<SessionType>>(session, ep)
                    ->handle_request();
                return;
            }

            // boost::urls::router router;

            std::cout << "path: " << path << std::endl;

            std::string prefix_tasks = "/tasks";
            // size_t pos = 0;

            // a long if else chain
            // auto cr = std::make_shared<server_async::ResponseCreator>(
            //     [&session](http::message_generator &&mg)
            //     {
            //         session->queue_write(std::move(mg));
            //     });

            std::shared_ptr<std::string const> const doc_root = std::make_shared<std::string>("."); // std::make_shared<std::string>(argv[3]);
            // you must consume the ep or destroy it by release. because parser don't support copy and assign. request object do.
            std::make_shared<server_async::FileRequestHandler<SessionType>>(session, *doc_root, ep)
                ->handle_request();

            // return

            std::cout << "Handling request..." << std::endl;
        }
    };
}
#endif
