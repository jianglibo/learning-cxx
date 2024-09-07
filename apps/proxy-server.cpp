#include "server-async.h"

int main(int argc, char *argv[])
{
    try
    {
        boost::asio::io_context io_context;
        tcp_server server(io_context, "192.168.0.90", "8000", true);
        io_context.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}