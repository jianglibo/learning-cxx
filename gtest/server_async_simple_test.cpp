#include <gtest/gtest.h>
#include "server-async-simple.h"

TEST(ServerAsync, PureProxy)
{
    try
    {
        boost::asio::io_context io_context;
        tcp_server ts = tcp_server::make_server(io_context, "192.168.0.90", "8000", 7890, "localhost", true);
        io_context.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}