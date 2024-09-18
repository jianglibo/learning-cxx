#pragma once

#ifndef SOCKET_COPY_H
#define SOCKET_COPY_H

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <optional>

using boost::asio::ip::tcp;

class socket_copier : public std::enable_shared_from_this<socket_copier>
{
public:
  using pointer = std::shared_ptr<socket_copier>;
  static pointer create(boost::asio::io_context &io_context,
                        const tcp::resolver::results_type &proxy_endpoints)
  {
    return pointer(new socket_copier(io_context, proxy_endpoints));
  }

  tcp::socket &socket() { return socket_; }

  void start()
  {

    auto self(shared_from_this());
    boost::asio::async_connect(remote_socket_, proxy_endpoints_,
                               [this, self](boost::system::error_code ec, const tcp::endpoint &)
                               {
                                 if (!ec)
                                 {
                                   //  std::cout << "Connected to proxy server done." << std::endl;
                                   do_read_client();
                                   do_read_proxy();
                                 }
                                 else
                                 {
                                   std::cerr << "Error connecting to proxy server: " << ec.message() << std::endl;
                                   cleanup();
                                 }
                               });
  };

private:
  socket_copier(boost::asio::io_context &io_context,
                const tcp::resolver::results_type &proxy_endpoints,
                //  const std::string &proxy_address,
                //  const std::string &proxy_port,
                bool debug_mode = false)
      : socket_(io_context),
        remote_socket_(io_context),
        resolver_(io_context),
        proxy_endpoints_(proxy_endpoints)
  {
  }

  bool has_complete_http_request(const std::string &request)
  {
    // A simple check to see if the request ends with \r\n\r\n (headers end)
    return request.find("\r\n\r\n") != std::string::npos;
  }

  std::string parse_http_request(const std::string &request);

  size_t get_content_length(const std::string &request)
  {
    std::istringstream stream(request);
    std::string line;
    size_t content_length = 0;

    // Search for the Content-Length header
    while (std::getline(stream, line) && line != "\r")
    {
      if (line.find("Content-Length:") != std::string::npos)
      {
        content_length = std::stoi(line.substr(16)); // Extract the Content-Length value
      }
    }

    return content_length;
  }

  void do_read_client()
  {
    // std::cout << "do_read_client get called." << std::endl;
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_client_, max_length),
                            [this, self](boost::system::error_code ec, std::size_t length)
                            {
                              // std::cout << "do_read_client get completed." << std::endl;
                              if (ec)
                              {
                                if (ec == boost::asio::error::eof)
                                {
                                  // Connection closed cleanly by the client
                                  // std::cout << "Client closed connection" << std::endl;
                                }
                                else
                                {
                                  std::cerr << "Error reading from client: " << ec.message() << std::endl;
                                }
                                cleanup();
                                return;
                              }
                              do_write_proxy(length);
                            });
  }

  void do_write_proxy(std::size_t length)
  {
    auto self(shared_from_this());
    if (length == 0)
    {
      do_read_client();
    }
    else
    {
      // data from client
      boost::asio::async_write(remote_socket_, boost::asio::buffer(data_client_, length),
                               [this, self](boost::system::error_code ec, std::size_t)
                               {
                                 if (!ec)
                                 {
                                   //  std::cout << "Data written to proxy server" << std::endl;
                                   //  do_read_proxy();
                                   do_read_client();
                                 }
                                 else
                                 {
                                   std::cerr << "Error writing to proxy server: " << ec.message() << std::endl;
                                   cleanup();
                                 }
                               });
    }
  }

  void do_read_proxy()
  {
    auto self(shared_from_this());
    remote_socket_.async_read_some(boost::asio::buffer(data_proxy_, max_length),
                                   [this, self](boost::system::error_code ec, std::size_t length)
                                   {
                                     if (ec)
                                     {
                                       if (ec == boost::asio::error::eof)
                                       {
                                         //  std::cout << "Proxy server closed connection" << std::endl;
                                       }
                                       else
                                       {
                                         std::cerr << "Error reading from proxy server: " << ec.message() << std::endl;
                                       }
                                       cleanup();
                                       return;
                                     }
                                     //  std::cout << "Data read from proxy server, length: " << length << std::endl;
                                     do_write_client(length);
                                   });
  }

  void do_write_client(std::size_t length)
  {
    auto self(shared_from_this());
    // data from proxy
    boost::asio::async_write(socket_, boost::asio::buffer(data_proxy_, length),
                             [this, self](boost::system::error_code ec, std::size_t)
                             {
                               if (!ec)
                               {
                                 //  std::cout << "Data written to client, read more from proxy" << std::endl;
                                 //  do_read_client();
                                 do_read_proxy();
                               }
                               else
                               {
                                 std::cerr << "Error writing to client: " << ec.message() << std::endl;
                                 cleanup();
                               }
                             });
  }

  void cleanup()
  {
    // Close both sockets
    boost::system::error_code ec;
    if (socket_.is_open())
      socket_.close(ec);
    if (remote_socket_.is_open())
      remote_socket_.close(ec);

    if (ec)
    {
      std::cerr << "Error during cleanup: " << ec.message() << std::endl;
    }
  }

  tcp::socket socket_;
  tcp::socket remote_socket_;
  tcp::resolver resolver_;
  tcp::resolver::results_type proxy_endpoints_;

  enum
  {
    max_length = 4096
  };
  char data_client_[max_length];
  char data_proxy_[max_length];
};
#endif
