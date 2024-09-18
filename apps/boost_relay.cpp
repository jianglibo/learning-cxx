#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>

namespace net = boost::asio;      // from <boost/asio.hpp>
namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

void relay_data(tcp::socket &client_socket, tcp::socket &server_socket)
{
    auto relay = [&client_socket, &server_socket](tcp::socket &src, tcp::socket &dest)
    {
        std::array<char, 4096> buffer;
        boost::system::error_code ec;
        size_t bytes_transferred = src.read_some(net::buffer(buffer), ec);

        if (!ec && bytes_transferred > 0)
        {
            dest.write_some(net::buffer(buffer.data(), bytes_transferred), ec);
        }
    };

    // Relay in both directions asynchronously
    std::thread t1([&]()
                   {
        while (true) relay(client_socket, server_socket); });
    std::thread t2([&]()
                   {
        while (true) relay(server_socket, client_socket); });

    // Join threads when done
    t1.join();
    t2.join();
}

class Handler
{
public:
    virtual void your_handler() = 0;

    // override operator ()
    void operator()()
    {
        your_handler();
    }

    void enable_connect_method()
    {
        handle_connect_method = true;
    }
    void enable_http_proxy_request()
    {
        handle_http_proxy_request = true;
    }

    void set_client_socket(tcp::socket client_socket)
    {
        this->client_socket = std::move(client_socket);
    }

private:
    tcp::socket client_socket;
    bool handle_connect_method = false;
    bool handle_http_proxy_request = false;
};

void handle_connection(tcp::socket client_socket)
{
    try
    {
        beast::flat_buffer buffer;
        http::request<http::string_body> req;

        // Read the incoming HTTP request
        http::read(client_socket, buffer, req);

        if (req.method() == http::verb::connect)
        {
            // Handle CONNECT request (for HTTPS tunneling)
            const std::string &target = req.target();
            size_t colon_pos = target.find(':');
            if (colon_pos == std::string::npos)
            {
                throw std::runtime_error("Invalid CONNECT request");
            }

            std::string host = target.substr(0, colon_pos);
            std::string port = target.substr(colon_pos + 1);

            // Connect to the target server
            tcp::resolver resolver(client_socket.get_executor());
            auto endpoints = resolver.resolve(host, port);
            tcp::socket server_socket(client_socket.get_executor());
            net::connect(server_socket, endpoints);

            // Send 200 OK response to the client
            http::response<http::empty_body> res{http::status::ok, req.version()};
            res.set(http::field::content_length, "0");
            http::write(client_socket, res);

            // Relay data between client and server
            relay_data(client_socket, server_socket);
        }
        else if (req.method() == http::verb::get)
        {
            // Handle GET request
            const std::string &target = req.target();
            if (target.find("http://") == 0 || target.find("https://") == 0)
            {
                // Parse the target URL to get host and path
                size_t start_pos = target.find("//") + 2;
                size_t end_pos = target.find('/', start_pos);
                std::string host = target.substr(start_pos, end_pos - start_pos);
                std::string path = target.substr(end_pos);

                // Resolve the host and make the request to the target server
                tcp::resolver resolver(client_socket.get_executor());
                auto endpoints = resolver.resolve(host, "80");
                tcp::socket server_socket(client_socket.get_executor());
                net::connect(server_socket, endpoints);

                // Send the GET request to the target server
                http::request<http::empty_body> forward_req{http::verb::get, path, req.version()};
                forward_req.set(http::field::host, host);
                forward_req.set(http::field::user_agent, req[http::field::user_agent]);
                http::write(server_socket, forward_req);

                // Read the response from the target server and send it to the client
                http::response<http::string_body> server_res;
                http::read(server_socket, buffer, server_res);
                http::write(client_socket, server_res);
            }
            else
            {
                // Handle other GET requests or return a 404 error
                http::response<http::string_body> res{http::status::bad_request, req.version()};
                res.set(http::field::content_length, "0");
                http::write(client_socket, res);
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}



int main()
{
    try
    {
        net::io_context ioc;

        tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 8080));
        while (true)
        {
            tcp::socket client_socket(ioc);
            acceptor.accept(client_socket);

            // Handle the connection asynchronously
            std::thread(&handle_connection, std::move(client_socket)).detach();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
