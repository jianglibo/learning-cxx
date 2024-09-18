#include <gtest/gtest.h>

class RequestHandlerBase
{
public:
    template <typename body_type>
    body_type handle_request()
    {
        std::cout << "RequestHandlerBase::handle_request(body_type) called with " << std::endl;
    }
    virtual ~RequestHandlerBase() = default;
};

class RequestHandler1 : public RequestHandlerBase
{
public:
    int handle_request()
    {
        std::cout << "RequestHandler1::handle_request(int) called with " << std::endl;
        return 0;
    }
};

class RequestHandler2 : public RequestHandlerBase
{
public:
    std::string handle_request()
    {
        std::cout << "RequestHandler2::handle_request(std::string) called with " << std::endl;
        return "0";
    }
};

TEST(PolymorphisamTest, Templated)
{
    RequestHandler1 rh1;
    RequestHandler2 rh2;
    std::vector<std::unique_ptr<RequestHandlerBase>> handlers;
    handlers.push_back(std::make_unique<RequestHandler1>(rh1));
    handlers.push_back(std::make_unique<RequestHandler2>(rh2));

    for (auto &hl : handlers)
    {
        // if hl has a handle_request method, then call it.

        // hl->handle_request();
    }
}
