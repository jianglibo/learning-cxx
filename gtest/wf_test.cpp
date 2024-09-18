#include <gtest/gtest.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include "workflow/HttpMessage.h"
#include "workflow/HttpUtil.h"
#include "workflow/WFServer.h"
#include "workflow/WFHttpServer.h"
#include "workflow/WFFacilities.h"
#include <utility>
#include "workflow/Workflow.h"
#include <fcntl.h>
#include <unistd.h>
#include "workflow/WFTaskFactory.h"
#include <iostream>
#include <filesystem>
#include <map>
#include <functional>
#include <regex>
#include <vector>
#include "workflow/WFOperator.h"

void echo_process(WFHttpTask *server_task)
{
    protocol::HttpRequest *req = server_task->get_req();
    protocol::HttpResponse *resp = server_task->get_resp();
    long long seq = server_task->get_task_seq();
    protocol::HttpHeaderCursor cursor(req);
    std::string name;
    std::string value;
    char buf[8192];
    int len;

    /* Set response message body. */
    resp->append_output_body_nocopy("<html>", 6);
    len = snprintf(buf, 8192, "<p>%s %s %s</p>", req->get_method(),
                   req->get_request_uri(), req->get_http_version());
    resp->append_output_body(buf, len);

    while (cursor.next(name, value))
    {
        len = snprintf(buf, 8192, "<p>%s: %s</p>", name.c_str(), value.c_str());
        resp->append_output_body(buf, len);
    }

    resp->append_output_body_nocopy("</html>", 7);

    /* Set status line if you like. */
    resp->set_http_version("HTTP/1.1");
    resp->set_status_code("200");
    resp->set_reason_phrase("OK");

    resp->add_header_pair("Content-Type", "text/html");
    resp->add_header_pair("Server", "Sogou WFHttpServer");
    if (seq == 9) /* no more than 10 requests on the same connection. */
        resp->add_header_pair("Connection", "close");

    /* print some log */
    char addrstr[128];
    struct sockaddr_storage addr;
    socklen_t l = sizeof addr;
    unsigned short port = 0;

    server_task->get_peer_addr((struct sockaddr *)&addr, &l);
    if (addr.ss_family == AF_INET)
    {
        struct sockaddr_in *sin = (struct sockaddr_in *)&addr;
        inet_ntop(AF_INET, &sin->sin_addr, addrstr, 128);
        port = ntohs(sin->sin_port);
    }
    else if (addr.ss_family == AF_INET6)
    {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&addr;
        inet_ntop(AF_INET6, &sin6->sin6_addr, addrstr, 128);
        port = ntohs(sin6->sin6_port);
    }
    else
        strcpy(addrstr, "Unknown");

    fprintf(stderr, "Peer address: %s:%d, seq: %lld.\n",
            addrstr, port, seq);
}

static WFFacilities::WaitGroup echo_wait_group(1);

void echo_sig_handler(int signo)
{
    echo_wait_group.done();
}

struct tutorial_series_context
{
    std::string url;
    WFHttpTask *proxy_task;
    bool is_keep_alive;
};

void reply_callback(WFHttpTask *proxy_task)
{
    SeriesWork *series = series_of(proxy_task);
    tutorial_series_context *context =
        (tutorial_series_context *)series->get_context();
    auto *proxy_resp = proxy_task->get_resp();
    size_t size = proxy_resp->get_output_body_size();

    if (proxy_task->get_state() == WFT_STATE_SUCCESS)
        fprintf(stderr, "%s: Success. Http Status: %s, BodyLength: %zu\n",
                context->url.c_str(), proxy_resp->get_status_code(), size);
    else /* WFT_STATE_SYS_ERROR*/
        fprintf(stderr, "%s: Reply failed: %s, BodyLength: %zu\n",
                context->url.c_str(), strerror(proxy_task->get_error()), size);
}

void proxy_http_callback(WFHttpTask *task)
{
    int state = task->get_state();
    int error = task->get_error();
    auto *resp = task->get_resp();
    SeriesWork *series = series_of(task);
    tutorial_series_context *context =
        (tutorial_series_context *)series->get_context();
    auto *proxy_resp = context->proxy_task->get_resp();

    if (state == WFT_STATE_SUCCESS)
    {
        const void *body;
        size_t len;

        /* set a callback for getting reply status. */
        context->proxy_task->set_callback(reply_callback);

        /* Copy the remote webserver's response, to proxy response. */
        resp->get_parsed_body(&body, &len);
        resp->append_output_body_nocopy(body, len);
        *proxy_resp = std::move(*resp);
        if (!context->is_keep_alive)
            proxy_resp->set_header_pair("Connection", "close");
    }
    else
    {
        const char *err_string;

        if (state == WFT_STATE_SYS_ERROR)
            err_string = strerror(error);
        else if (state == WFT_STATE_DNS_ERROR)
            err_string = gai_strerror(error);
        else if (state == WFT_STATE_SSL_ERROR)
            err_string = "SSL error";
        else /* if (state == WFT_STATE_TASK_ERROR) */
            err_string = "URL error (Cannot be a HTTPS proxy)";

        fprintf(stderr, "%s: Fetch failed. state = %d, error = %d: %s\n",
                context->url.c_str(), state, error, err_string);

        /* As a tutorial, make it simple. And ignore reply status. */
        proxy_resp->set_status_code("404");
        proxy_resp->append_output_body_nocopy(
            "<html>404 Not Found.</html>", 27);
    }
}

void proxy_process(WFHttpTask *proxy_task)
{
    auto *req = proxy_task->get_req();
    SeriesWork *series = series_of(proxy_task);
    WFHttpTask *http_task; /* for requesting remote webserver. */

    tutorial_series_context *context = new tutorial_series_context;
    context->url = req->get_request_uri();
    context->proxy_task = proxy_task;

    series->set_context(context);
    series->set_callback([](const SeriesWork *series)
                         { delete (tutorial_series_context *)series->get_context(); });

    context->is_keep_alive = req->is_keep_alive();
    http_task = WFTaskFactory::create_http_task(req->get_request_uri(), 0, 0,
                                                proxy_http_callback);

    const void *body;
    size_t len;

    /* Copy user's request to the new task's reuqest using std::move() */
    req->set_request_uri(http_task->get_req()->get_request_uri());
    req->get_parsed_body(&body, &len);
    req->append_output_body_nocopy(body, len);
    *http_task->get_req() = std::move(*req);

    /* also, limit the remote webserver response size. */
    http_task->get_resp()->set_size_limit(200 * 1024 * 1024);

    *series << http_task;
}

static WFFacilities::WaitGroup proxy_wait_group(1);

void proxy_sig_handler(int signo)
{
    proxy_wait_group.done();
}

using namespace protocol;

void file_pread_callback(WFFileIOTask *task)
{
    FileIOArgs *args = task->get_args();
    long ret = task->get_retval();
    HttpResponse *resp = (HttpResponse *)task->user_data;

    close(args->fd);
    if (task->get_state() != WFT_STATE_SUCCESS || ret < 0)
    {
        resp->set_status_code("503");
        resp->append_output_body("<html>503 Internal Server Error.</html>");
    }
    else /* Use '_nocopy' carefully. */
        resp->append_output_body_nocopy(args->buf, ret);
}

void file_process(WFHttpTask *server_task, const char *root)
{
    HttpRequest *req = server_task->get_req();
    HttpResponse *resp = server_task->get_resp();
    const char *uri = req->get_request_uri();
    const char *p = uri;

    printf("Request-URI: %s\n", uri);
    while (*p && *p != '?')
        p++;

    std::string abs_path(uri, p - uri);
    abs_path = root + abs_path;
    if (abs_path.back() == '/')
        abs_path += "index.html";

    resp->add_header_pair("Server", "Sogou C++ Workflow Server");

    int fd = open(abs_path.c_str(), O_RDONLY);
    if (fd >= 0)
    {
        size_t size = lseek(fd, 0, SEEK_END);
        void *buf = malloc(size); /* As an example, assert(buf != NULL); */
        WFFileIOTask *pread_task;

        pread_task = WFTaskFactory::create_pread_task(fd, buf, size, 0,
                                                      file_pread_callback);
        /* To implement a more complicated server, please use series' context
         * instead of tasks' user_data to pass/store internal data. */
        pread_task->user_data = resp; /* pass resp pointer to pread task. */
        server_task->user_data = buf; /* to free() in callback() */
        server_task->set_callback([](WFHttpTask *t)
                                  { free(t->user_data); });
        series_of(server_task)->push_back(pread_task);
    }
    else
    {
        resp->set_status_code("404");
        // convert abs_path to absolute path
        std::filesystem::path absolute_path = std::filesystem::absolute(abs_path);
        // /home/jianglibo/learning-cxx/build/gtest/
        std::ostringstream oss;
        oss << "<html>404 Not Found. " << abs_path << ", " << absolute_path << "</html>";
        std::cout << oss.str() << std::endl;
        resp->append_output_body(oss.str());
    }
}

static WFFacilities::WaitGroup file_wait_group(1);

void file_sig_handler(int signo)
{
    file_wait_group.done();
}

class Router
{
public:
    using Handler = std::function<void(WFHttpTask *, const std::string &, const std::map<std::string, std::string> &)>;

    // Register a route with dynamic pattern support
    void register_route(const std::string &uri_pattern, Handler handler)
    {
        route_map.push_back({uri_pattern, handler});
    }

    // Function to process the incoming request and dispatch it to the right handler
    void process_request(WFHttpTask *task, const std::string &uri)
    {
        for (const auto &[pattern, handler] : route_map)
        {
            std::map<std::string, std::string> params;
            if (match_route(pattern, uri, params))
            {
                handler(task, uri, params); // Call the handler with extracted parameters
                return;
            }
        }
        handle_404(task, uri); // If no match, return 404
    }

private:
    // List of routes and their handlers
    std::vector<std::pair<std::string, Handler>> route_map;

    // Default handler for 404 errors
    void handle_404(WFHttpTask *task, const std::string &uri)
    {
        HttpResponse *resp = task->get_resp();
        resp->set_status_code("404");
        std::ostringstream oss;
        oss << "<html><body>404 Not Found: " << uri << "</body></html>";
        resp->append_output_body(oss.str());
        task->set_callback([](WFHttpTask *t) {});
    }

    // Dynamic route matching function
    bool match_route(const std::string &pattern, const std::string &uri, std::map<std::string, std::string> &params)
    {
        std::vector<std::string> pattern_parts = split(pattern, '/');
        std::vector<std::string> uri_parts = split(uri, '/');

        if (pattern_parts.size() != uri_parts.size())
            return false;

        for (size_t i = 0; i < pattern_parts.size(); ++i)
        {
            if (pattern_parts[i].empty())
                continue;

            if (pattern_parts[i][0] == ':')
            {
                // This part is a dynamic parameter
                std::string param_name = pattern_parts[i].substr(1);
                params[param_name] = uri_parts[i]; // Capture the parameter value
            }
            else if (pattern_parts[i] != uri_parts[i])
            {
                // Static part of the route doesn't match
                return false;
            }
        }
        return true;
    }

    // Helper function to split strings by a delimiter
    std::vector<std::string> split(const std::string &str, char delimiter)
    {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(str);
        while (std::getline(tokenStream, token, delimiter))
        {
            tokens.push_back(token);
        }
        return tokens;
    }
};

// void process_http_connect(WFHttpTask *task)
// {
//     protocol::HttpRequest *req = task->get_req();
//     protocol::HttpResponse *resp = task->get_resp();

//     // Parse the CONNECT request to extract the target host and port
//     std::string target_host;
//     int target_port = 443; // default port for HTTPS

//     const std::string &method = req->get_method();

//     if (method == "CONNECT")
//     {
//         protocol::HttpHeaderCursor cursor(req);
//         std::string host_name{"Host"};
//         std::string host_str;
//         cursor.find(host_name, host_str); // target in "Host: example.com:443"

//         // Split host and port if needed (use string parsing logic)
//         size_t pos = host_str.find(":");
//         if (pos != std::string::npos)
//         {
//             target_host = host_str.substr(0, pos);
//             target_port = std::stoi(host_str.substr(pos + 1));
//         }
//         else
//         {
//             target_host = host_str;
//         }

//         // Now establish the raw TCP connection to the target
//         WFHttpServerTask *proxy_task = static_cast<WFHttpServerTask *>(task);

//         // Initiate raw connection to target_host:target_port
//         WFGoTask *tcp_task = WFTaskFactory::create_go_task("connect_to_target",
//                                                            [proxy_task, target_host, target_port]
//                                                            {
//                                                                // Step 1: Create raw TCP connection to the target server
//                                                                int sockfd = socket(AF_INET, SOCK_STREAM, 0);
//                                                                if (sockfd < 0)
//                                                                {
//                                                                    proxy_task->get_resp()->set_status_code("502 Bad Gateway");
//                                                                    return;
//                                                                }

//                                                                // Set up sockaddr_in with target_host and target_port
//                                                                struct sockaddr_in server_addr;
//                                                                memset(&server_addr, 0, sizeof(server_addr));
//                                                                server_addr.sin_family = AF_INET;
//                                                                server_addr.sin_port = htons(target_port);

//                                                                // Resolve hostname
//                                                                struct hostent *server = gethostbyname(target_host.c_str());
//                                                                if (server == NULL)
//                                                                {
//                                                                    proxy_task->get_resp()->set_status_code("502 Bad Gateway");
//                                                                    close(sockfd);
//                                                                    return;
//                                                                }
//                                                                memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

//                                                                // Step 2: Connect to the target server
//                                                                if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
//                                                                {
//                                                                    proxy_task->get_resp()->set_status_code("502 Bad Gateway");
//                                                                    close(sockfd);
//                                                                    return;
//                                                                }
//                                                                // Step 3: Send the "200 Connection Established" response back to the client
//                                                                proxy_task->get_resp()->set_status_code("200 Connection Established");
//                                                                proxy_task->get_resp()->append_output_body("Connection established\r\n");

//                                                                // Step 4: Relay data between the client and server (TCP forwarding)
//                                                                // This part requires setting up bidirectional forwarding for data flow.
//                                                                // For simplicity, the logic to forward data can be implemented here.
//                                                                // Use `recv()` and `send()` for data forwarding.
//                                                            });
//         // Start the TCP connection task
//         tcp_task->start();
//     }
//     else
//     {
//         // Handle other HTTP methods like GET, POST, etc.
//         resp->append_output_body("<html><body><h1>Hello World</h1></body></html>");
//         resp->set_status_code("200 OK");
//     }
// }

#define BUFFER_SIZE 4096

void relay_data(int client_fd, int server_fd)
{
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_sent;

    // Relay data from client to server
    while ((bytes_read = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0)
    {
        bytes_sent = send(server_fd, buffer, bytes_read, 0);
        if (bytes_sent <= 0)
        {
            break; // Error occurred while sending to server
        }
    }

    // Relay data from server to client
    while ((bytes_read = recv(server_fd, buffer, BUFFER_SIZE, 0)) > 0)
    {
        bytes_sent = send(client_fd, buffer, bytes_read, 0);
        if (bytes_sent <= 0)
        {
            break; // Error occurred while sending to client
        }
    }

    // Close the connections when done
    close(client_fd);
    close(server_fd);
}

void process_http_connect(WFHttpTask *task)
{
    protocol::HttpRequest *req = task->get_req();
    protocol::HttpResponse *resp = task->get_resp();

    // Parse the CONNECT request to extract the target host and port
    std::string target_host;
    int target_port = 443; // default port for HTTPS

    const std::string &method = req->get_method();
    if (method == "CONNECT")
    {
        protocol::HttpHeaderCursor cursor(req);
        std::string host_name{"Host"};
        std::string host_str;
        cursor.find(host_name, host_str); // target in "Host: example.com:443"

        // Split host and port if needed (use string parsing logic)
        size_t pos = host_str.find(":");
        if (pos != std::string::npos)
        {
            target_host = host_str.substr(0, pos);
            target_port = std::stoi(host_str.substr(pos + 1));
        }
        else
        {
            target_host = host_str;
        }

        // Create and initiate a TCP connection task to the target
        WFGoTask *tcp_task = WFTaskFactory::create_go_task("connect_to_target",
                                                           [target_host, target_port, task]()
                                                           {
                                                               // Prepare variables to hold the peer address
                                                               struct sockaddr_in peer_addr; // Assuming it's an IPv4 address
                                                               socklen_t addrlen = sizeof(peer_addr);

                                                               int client_fd = task->get_peer_addr((struct sockaddr*)&peer_addr, &addrlen); // Get the client socket

                                                               int server_fd = socket(AF_INET, SOCK_STREAM, 0);
                                                               if (server_fd < 0)
                                                               {
                                                                   // Connection failure, handle it
                                                                   task->get_resp()->set_status_code("502 Bad Gateway");
                                                                   task->noreply(); // End task
                                                                   return;
                                                               }

                                                               // Set up sockaddr_in with target_host and target_port
                                                               struct sockaddr_in server_addr;
                                                               memset(&server_addr, 0, sizeof(server_addr));
                                                               server_addr.sin_family = AF_INET;
                                                               server_addr.sin_port = htons(target_port);

                                                               struct hostent *server = gethostbyname(target_host.c_str());
                                                               if (server == NULL)
                                                               {
                                                                   close(server_fd);
                                                                   task->get_resp()->set_status_code("502 Bad Gateway");
                                                                   task->noreply(); // End task
                                                                   return;
                                                               }
                                                               memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

                                                               if (connect(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
                                                               {
                                                                   close(server_fd);
                                                                   task->get_resp()->set_status_code("502 Bad Gateway");
                                                                   task->noreply(); // End task
                                                                   return;
                                                               }

                                                               // Send "200 Connection Established" to the client
                                                               task->get_resp()->set_status_code("200 Connection Established");
                                                               task->get_resp()->append_output_body("Connection established\r\n");
                                                               task->noreply(); // Send response back to the client

                                                               // Relay data between the client and the server
                                                               relay_data(client_fd, server_fd);
                                                           });

        // Start the TCP connection task
        tcp_task->start();
    }
    else
    {
        // Handle other HTTP methods like GET, POST, etc.
        resp->append_output_body("<html><body><h1>Hello World</h1></body></html>");
        resp->set_status_code("200 OK");
    }
}

TEST(WfTest, TaskAndHeader)
{
    WFHttpTask *http_task = WFTaskFactory::create_http_task("/", 0, 0, nullptr);
    const void *body;
    size_t len;
    protocol::HttpRequest *req = http_task->get_req();
    req->add_header_pair("Host", "www.example.com");
    protocol::HttpHeaderCursor cursor(req);
    std::string name;
    std::string value;
    std::string value1;
    cursor.find("Host", value);
    ASSERT_STREQ(value.c_str(), "www.example.com");
    cursor.find("Noexist", value1);
    ASSERT_STREQ(value1.c_str(), "");
}

TEST(WfTest, Connect)
{
    WFHttpServer server(process_http_connect);
    server.start(8080); // Start the server on port 8080

    getchar(); // Keep the server running
    server.stop();
}

TEST(WfTest, Echo)
{
    unsigned short port = 3001;
    signal(SIGINT, echo_sig_handler);
    WFHttpServer server(echo_process);
    if (server.start(port) == 0)
    {
        echo_wait_group.wait();
        server.stop();
    }
    else
    {
        perror("Cannot start server");
        exit(1);
    }
}

TEST(WfTest, Proxy)
{

    unsigned short port = 3001;
    signal(SIGINT, proxy_sig_handler);
    struct WFServerParams params = HTTP_SERVER_PARAMS_DEFAULT;
    /* for safety, limit request size to 8MB. */
    params.request_size_limit = 8 * 1024 * 1024;
    WFHttpServer server(&params, proxy_process);
    if (server.start(port) == 0)
    {
        proxy_wait_group.wait();
        server.stop();
    }
    else
    {
        perror("Cannot start server");
        exit(1);
    }
}

TEST(WfTest, File)
{

    signal(SIGINT, file_sig_handler);

    unsigned short port = 3001;
    const char *root = ".";
    auto &&proc = std::bind(file_process, std::placeholders::_1, root);
    WFHttpServer server(proc);
    std::string scheme;
    int ret;

    // if (argc == 5)
    // {
    //     ret = server.start(port, argv[3], argv[4]); /* https server */
    //     scheme = "https://";
    // }
    // else
    // {
    ret = server.start(port);
    scheme = "http://";
    // }

    if (ret < 0)
    {
        perror("start server");
        exit(1);
    }

    /* Test the server. */
    // auto &&create = [&scheme, port](WFRepeaterTask *) -> SubTask *
    // {
    //     char buf[1024];
    //     *buf = '\0';
    //     printf("Input file name: (Ctrl-D to exit): ");
    //     scanf("%1023s", buf);
    //     if (*buf == '\0')
    //     {
    //         printf("\n");
    //         return NULL;
    //     }

    //     std::string url = scheme + "127.0.0.1:" + std::to_string(port) + "/" + buf;
    //     WFHttpTask *task = WFTaskFactory::create_http_task(url, 0, 0,
    //                                                        [](WFHttpTask *task)
    //                                                        {
    //                                                            auto *resp = task->get_resp();
    //                                                            if (strcmp(resp->get_status_code(), "200") == 0)
    //                                                            {
    //                                                                std::string body = protocol::HttpUtil::decode_chunked_body(resp);
    //                                                                fwrite(body.c_str(), body.size(), 1, stdout);
    //                                                                printf("\n");
    //                                                            }
    //                                                            else
    //                                                            {
    //                                                                printf("%s %s\n", resp->get_status_code(), resp->get_reason_phrase());
    //                                                            }
    //                                                        });

    //     return task;
    // };

    // WFFacilities::WaitGroup wg(1);
    // WFRepeaterTask *repeater;
    // repeater = WFTaskFactory::create_repeater_task(create, [&wg](WFRepeaterTask *)
    //                                                { wg.done(); });

    // repeater->start();
    // wg.wait();
    file_wait_group.wait();
    server.stop();
}
