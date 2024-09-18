#pragma once

#ifndef SERVER_ASYNC_H
#define SERVER_ASYNC_H

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <optional>

using boost::asio::ip::tcp;

class tcp_connection : public std::enable_shared_from_this<tcp_connection>
{
public:
  using pointer = std::shared_ptr<tcp_connection>;

  static pointer create(boost::asio::io_context &io_context,
                        const tcp::resolver::results_type &proxy_endpoints,
                        // const std::string &proxy_address,
                        // const std::string &proxy_port,
                        bool debug_mode = false)
  {
    // return pointer(new tcp_connection(io_context, proxy_address, proxy_port, debug_mode));
    return pointer(new tcp_connection(io_context, proxy_endpoints, debug_mode));
  }

  tcp::socket &socket() { return socket_; }

  void start();

private:
  tcp_connection(boost::asio::io_context &io_context,
                 const tcp::resolver::results_type &proxy_endpoints,
                 //  const std::string &proxy_address,
                 //  const std::string &proxy_port,
                 bool debug_mode = false)
      : socket_(io_context),
        remote_socket_(io_context),
        resolver_(io_context),
        proxy_endpoints_(proxy_endpoints),
        // proxy_address_(proxy_address),
        // proxy_port_(proxy_port),
        debug_mode(debug_mode),
        header_state_(HeaderState::READING_HEADERS)
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

  void do_read_client();

  void do_write_proxy(std::size_t length);

  void do_read_proxy();

  void do_write_client(std::size_t length);

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

  enum class HeaderState
  {
    READING_HEADERS,
    READING_BODY
  };

  tcp::socket socket_;
  tcp::socket remote_socket_;
  tcp::resolver resolver_;
  std::string request_buffer_;
  bool debug_mode;
  HeaderState header_state_;
  tcp::resolver::results_type proxy_endpoints_;

  enum
  {
    max_length = 4096
  };
  char data_client_[max_length];
  char data_proxy_[max_length];
};

class tcp_server
{
public:
  // static make server
  inline static tcp_server make_server(boost::asio::io_context &io_context,
                                       const std::string &proxy_address,
                                       const std::string &proxy_port,
                                       const int port,
                                       const std::string &host = "",
                                       bool debug_mode = false)
  {
    if (host.empty())
    {
      return tcp_server(io_context, proxy_address, proxy_port, tcp::endpoint(tcp::v4(), port), debug_mode);
    }
    else
    {
      boost::asio::ip::address addr;
      try
      {
        addr = boost::asio::ip::make_address(host);
      }
      catch (const std::exception &e)
      {
        auto endpoints = tcp::resolver(io_context).resolve(host, "0");
        tcp::endpoint endpoint = *endpoints.begin();
        addr = endpoint.address();
      }
      return tcp_server(io_context, proxy_address, proxy_port, tcp::endpoint(addr, port), debug_mode);
    }
  }

private:
  tcp_server(boost::asio::io_context &io_context,
             const std::string &proxy_address,
             const std::string &proxy_port,
             const tcp::endpoint &endpoint,
             bool debug_mode = false)
      : io_context_(io_context),
        acceptor_(io_context, endpoint),
        proxy_address_(proxy_address),
        proxy_port_(proxy_port),
        resolver_(io_context),
        debug_mode(debug_mode)
  {
    proxy_endpoints_ = resolver_.resolve(proxy_address_, proxy_port_);
    start_accept();
  }
  void start_accept();
  boost::asio::io_context &io_context_;
  tcp::acceptor acceptor_;
  std::string proxy_address_;
  std::string proxy_port_;
  tcp::resolver resolver_;
  tcp::resolver::results_type proxy_endpoints_;
  bool debug_mode;
};

#endif
