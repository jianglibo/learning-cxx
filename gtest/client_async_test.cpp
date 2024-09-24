#include <gtest/gtest.h>
#include "http_client_async.hpp"
#include "server_async.h"
#include <boost/url.hpp>
#include <filesystem>
#include "http_server_async.hpp"
#include "models.hpp"
#include "json_util.hpp"

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

// void split_print_cxx14(http::request_serializer<http::empty_body> const &sr)
// {
//     boost::system::error_code ec;
//     // serializer<isRequest, Body, Fields> sr{m};
//     std::cout << "Header:" << std::endl;
//     do
//     {
//         sr.next(ec,
//                 [&sr](boost::system::error_code &ec, auto const &buffer)
//                 {
//                     ec = {};
//                     std::cout << make_printable(buffer);
//                     sr.consume(beast::buffer_bytes(buffer));
//                 });
//     } while (!sr.is_header_done());
//     if (!ec && !sr.is_done())
//     {
//         std::cout << "Body:" << std::endl;
//         do
//         {
//             sr.next(ec,
//                     [&sr](boost::system::error_code &ec, auto const &buffer)
//                     {
//                         ec = {};
//                         std::cout << make_printable(buffer);
//                         sr.consume(beast::buffer_bytes(buffer));
//                     });
//         } while (!ec && !sr.is_done());
//     }
//     if (ec)
//         std::cerr << ec.message() << std::endl;
// }

TEST(SerializerTest, RequestSerializer)
{
    std::string target = "/";
    std::string host = "www.example.com";
    http::request<http::empty_body> req{http::verb::get, target, 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.target("https://www.abc.com/hello");

    // http::request_serializer<http::empty_body> sr{req};
    // sr.split(true);
    // split_print_cxx14(sr);
    // std::cout << req << std::endl;

    // Create a flat_buffer to store the serialized headers
    // beast::flat_buffer buffer;

    // Use beast::flat_buffer to hold the request
    beast::flat_buffer buffer;

    // Create a string to hold the serialized request
    // std::string request_string;
    // http::request_serializer<http::empty_body> sr{req};
    // do
    // {
    //     // This call guarantees it will make some
    //     // forward progress, or otherwise return an error.
    //     beast::write_some(stream, sr);
    // } while (!sr.is_done());
    beast::error_code ec;
    server_async::write_ostream(std::cout, req, ec);

    std::cout
        << "Serialized Headers (raw bytes):\n";
    std::cout << boost::beast::buffers_to_string(buffer.data()) << std::endl;
}

TEST(BufferTest, FlatBuffer)
{

    const char *ch = "你好。";
    ASSERT_EQ(std::strlen(ch), 9);
    const char *some_data = "Hello, beast::flat_buffer!";
    std::size_t data_size = std::strlen(some_data);

    // Create a flat_buffer
    beast::flat_buffer buffer;

    // 1. Reserve space for the data in the buffer
    auto b = buffer.prepare(data_size); // Reserving 'data_size' bytes

    // 2. Copy data into the reserved space
    std::memcpy(b.data(), some_data, data_size); // Copy the data to the reserved space

    // 3. Commit the data, marking it as valid
    buffer.commit(data_size);

    // Now let's pretend we're reading the data from the buffer
    // 4. Access the committed data (in the buffer's readable region)
    std::cout << "Data in buffer: " << boost::beast::buffers_to_string(buffer.data()) << std::endl;
    std::cout << "Data in buffer again: " << boost::beast::buffers_to_string(buffer.data()) << std::endl;

    // 5. Consume part or all of the data (e.g., processing it)
    // Consume the exact number of bytes that we copied
    buffer.consume(data_size);

    // After consuming the data, the buffer should be empty
    std::cout << "Buffer size after consuming: " << buffer.size() << std::endl;
}

TEST(UrlTest, UrlNoScheme)
{
    std::string url = "//www.example.com";
    boost::urls::url_view url_view{url};

    ASSERT_FALSE(url_view.has_scheme());

    boost::system::result<boost::urls::url> ru = boost::urls::parse_uri_reference(url);
    ASSERT_FALSE(ru.has_error());
    if (ru.has_error())
    {
        std::cerr << "Error: " << ru.error().message() << std::endl;
    }

    ASSERT_STREQ(ru.value().host().data(), "www.example.com");

    ASSERT_STREQ(url_view.host().data(), "www.example.com");
    ASSERT_STREQ(url_view.port().data(), "");

    ru = boost::urls::parse_origin_form("/abc?txt");
    ASSERT_FALSE(ru.has_error());
    ASSERT_STREQ(ru.value().path().data(), "/abc");
    ASSERT_STREQ(ru.value().query().data(), "txt");
}

TEST(ServerTest, start)
{
    net::ip::address address = net::ip::make_address("0.0.0.0");                            // net::ip::make_address(argv[1]);
    unsigned short port{8080};                                                              // static_cast<unsigned short>(std::atoi(argv[2]));
    std::shared_ptr<std::string const> const doc_root = std::make_shared<std::string>("."); // std::make_shared<std::string>(argv[3]);
    auto const threads = 1;                                                                 // std::max<int>(1, std::atoi(argv[4]));
    server_async::HttpServer server(address, port, doc_root, threads);

    std::filesystem::path cwd = std::filesystem::current_path();
    std::cout << "Current working directory: " << cwd << std::endl;

    const char *cert_filepath = "../../apps/fixtures/cert.pem";
    const char *key_filepath = "../../apps/fixtures/key.pem";
    const char *dh_filepath = "../../apps/fixtures/dh.pem";

    // print out current directory absolutely

    server_async::SSLCertHolder ssl_cert_holder{server_async::read_whole_file(cert_filepath),
                                                server_async::read_whole_file(key_filepath),
                                                server_async::read_whole_file(dh_filepath)};

    // server_async::HandlerFunc<server_async::plain_http_session> plain_handler =
    //     std::bind(&handler<server_async::plain_http_session>, std::placeholders::_1, std::placeholders::_2);
    // server_async::HandlerFunc<server_async::ssl_http_session> ssl_handler =
    //     std::bind(&handler<server_async::ssl_http_session>, std::placeholders::_1, std::placeholders::_2);
    // server_async::HandlerCommon handler_common = [&doc_root](server_async::EmptyBodyParser ebr,
    //                                                          server_async::SessionVariant sv)
    // {
    //     http::response<http::string_body> res{http::status::ok, ebr.version()};
    //     res.keep_alive(ebr.keep_alive());

    //     auto file_res = std::make_shared<server_async::FileRequestHandler>(*doc_root, std::move(ebr))->handle_request();
    //     std::visit([&file_res](auto &&arg)
    //                { arg->queue_write(std::move(file_res)); },
    //                sv);
    // };

    handler<server_async::plain_http_session> plain_handler;
    handler<server_async::ssl_http_session> ssl_handler;

    std::thread t([&server, &ssl_cert_holder, &plain_handler, &ssl_handler]()
                  { server.start(ssl_cert_holder, plain_handler, ssl_handler); });

    std::this_thread::sleep_for(std::chrono::seconds(3));
    server.stop();
    t.join();
}

TEST(ModelsTest, JsonResponse)
{
    server_async::ResponseData responseData{"123"};
    server_async::ResponseData responseData1 = responseData;
    server_async::ApiResponseSuccess successResponse("Resource created successfully.", "2023-09-24T15:30:45Z", "2023-09-24T15:30:45Z", responseData);

    // Example of an error response
    std::vector<server_async::Error> errorList = {
        {"name", "ERR_NAME_REQUIRED", "Name is required."},
        {"email", "ERR_EMAIL_INVALID", "Email format is invalid."}};
    server_async::ApiResponseError errorResponse("Validation failed.", "2023-09-24T15:30:45Z", "2023-09-24T15:30:45Z", errorList);

    std::cout << "start json::value_from" << std::endl;
    json::value jv = json::value_from(errorResponse);
    server_async::pretty_print(std::cout, jv);

    jv = json::value_from(successResponse);
    server_async::pretty_print(std::cout, jv);

    namespace json = boost::json;
    jv = json::object{};
    jv.as_object()["data"] = 1;
    std::cout << jv << "\n";
    ASSERT_TRUE(jv.as_object().contains("data"));
}