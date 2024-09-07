#include <gtest/gtest.h>
#include "server-async.h"

TEST(ServerAsync, PureProxy)
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