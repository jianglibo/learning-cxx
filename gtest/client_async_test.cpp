#include <gtest/gtest.h>
#include "http_client_async.hpp"

TEST(BoostBeastClientTest, HTTP_BLOCK)
{

    client_async::ClientPoolSsl client_pool(3);
    // start in backgroud, using thread
    std::thread t([&client_pool]()
                  { client_pool.start(); });
    std::shared_ptr<client_async::session> client = client_pool.createSession();
    client->run("www.example.com", "80", "/", 11);
    auto res = client->get_response_future().get();
    std::cout << res << std::endl;
    // sleep 3 seconds
    // std::this_thread::sleep_for(std::chrono::seconds(3));
    client_pool.stop();
    t.join();
}
TEST(BoostBeastClientTest, HTTP_NOT_BLOCK)
{
    std::cout << "client_pool stopped" << std::endl;
    client_async::ClientPoolSsl client_pool(3);
    // start in backgroud, using thread
    client_pool.start(false);
    auto client = client_pool.createSession();
    client->run("www.example.com", "80", "/", 11);
    auto res = client->get_response_future().get();
    std::cout << res << std::endl;
    // sleep 3 seconds
    // std::this_thread::sleep_for(std::chrono::seconds(3));
    client_pool.stop();
}