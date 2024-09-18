#include "server-async-simple.h"

int main(int argc, char *argv[])
{
    try
    {
        boost::asio::io_context io_context;
        tcp_server tc = tcp_server::make_server(io_context, "192.168.0.90", "8000", 7890, "localhost", true);
        std::cout << "started at: " << "http://localhost:7890";
        io_context.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}