#include "server_async.h"
#include <fstream>
#include <iostream>

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
    server.start(server_async::read_whole_file(cert_filepath),
                 server_async::read_whole_file(key_filepath),
                 server_async::read_whole_file(dh_filepath));

    return EXIT_SUCCESS;
}