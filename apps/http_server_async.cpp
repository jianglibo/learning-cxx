#include "server_async.h"

void relay_data(tcp::socket &client_socket, tcp::socket &server_socket, net::strand<net::io_context::executor_type> &strand)
{
    auto buffer = std::make_shared<std::array<char, 4096>>();

    auto relay = [&client_socket, &server_socket, buffer, &strand](tcp::socket &source, tcp::socket &dest)
    {
        // Asynchronous read from the source socket
        source.async_read_some(
            net::buffer(*buffer),
            net::bind_executor(
                strand, // Ensure this operation is serialized in the strand
                [&dest, buffer](boost::system::error_code ec, std::size_t bytes_transferred)
                {
                    if (!ec)
                    {
                        // Asynchronously write the data to the destination socket
                        net::async_write(
                            dest,
                            net::buffer(buffer->data(), bytes_transferred),
                            [&dest](boost::system::error_code ec, std::size_t /*length*/)
                            {
                                if (ec)
                                {
                                    dest.close(); // Close the destination socket on error
                                }
                            });
                    }
                    else
                    {
                        dest.close(); // Close the destination socket on error
                    }
                }));
    };

    // Start relaying data in both directions
    relay(client_socket, server_socket); // Client to Server
    relay(server_socket, client_socket); // Server to Client
}

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
    server.start();
    return EXIT_SUCCESS;
}