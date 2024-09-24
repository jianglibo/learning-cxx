#include "server_async.h"
#include <fstream>
#include <iostream>
#include "http_server_async.hpp"

// http::message_generator handler_common(server_async::EmptyBodyParser ebr)
// {
//     return http::response<http::string_body>(http::status::ok, ebr.version());
// }

// http::message_generator plain_handler()
// {
// }
// http::message_generator ssl_handler(std::shared_ptr<server_async::ssl_http_session> http_session, server_async::EmptyBodyParser ebr)
// {
//     return http::response<http::string_body>(http::status::ok, ebr.version());
// }

int main(int argc, char *argv[])
{
    // Check command line arguments.
    if (argc != 5)
    {
        std::cerr << "Usage: advanced-server-flex <address> <port> <doc_root> <threads>\n"
                  << "Example:\n"
                  << "    advanced-server-flex 0.0.0.0 8080 . 1\n";
        return EXIT_FAILURE;
    }
    auto const address = net::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    auto const doc_root = std::make_shared<std::string>(argv[3]);
    auto const threads = std::max<int>(1, std::atoi(argv[4]));
    server_async::HttpServer server(address, port, doc_root, threads);

    const char *cert_filepath = "apps/fixtures/cert.pem";
    const char *key_filepath = "apps/fixtures/key.pem";
    const char *dh_filepath = "apps/fixtures/dh.pem";

    server_async::SSLCertHolder ssl_cert_holder{server_async::read_whole_file(cert_filepath),
                                                server_async::read_whole_file(key_filepath),
                                                server_async::read_whole_file(dh_filepath)};


    // server_async::HandlerFunc<server_async::plain_http_session> plain_handler =
    //     std::bind(&handler<server_async::plain_http_session>, std::placeholders::_1, std::placeholders::_2);
    // server_async::HandlerFunc<server_async::ssl_http_session> ssl_handler =
    //     std::bind(&handler<server_async::ssl_http_session>, std::placeholders::_1, std::placeholders::_2);
    // auto plain_handler =
    //     std::bind(&handler<server_async::plain_http_session>, std::placeholders::_1, std::placeholders::_2);
    // auto ssl_handler =
    //     std::bind(&handler<server_async::ssl_http_session>, std::placeholders::_1, std::placeholders::_2);
    handler<server_async::plain_http_session> plain_handler{};
    handler<server_async::ssl_http_session> ssl_handler{};
    server.start(ssl_cert_holder, plain_handler, ssl_handler);

    return EXIT_SUCCESS;
}