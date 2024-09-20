#pragma once

#ifndef HTTP_COPY_H
#define HTTP_COPY_H

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <optional>
#include <boost/url.hpp>

using boost::asio::ip::tcp;

class http_copier : public std::enable_shared_from_this<http_copier>
{
public:
  using pointer = std::shared_ptr<http_copier>;
  static pointer create(
      tcp::socket &&socket_,
      http::request<http::empty_body> &&req_,
      beast::flat_buffer &&buffer_)
  {
    return pointer(new http_copier(std::forward<tcp::socket>(socket_),
                                   std::forward<http::request<http::empty_body>>(req_),
                                   std::forward<beast::flat_buffer>(buffer_)));
  }

  void do_relay(tcp::socket &source, tcp::socket &dest, std::shared_ptr<std::array<char, 4096>> &buffer)
  {
    // Asynchronous read from the source socket
    source.async_read_some(
        net::buffer(*buffer),
        net::bind_executor(
            source.get_executor(),
            [&source, &dest, &buffer, self = shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred)
            {
              if (!ec)
              {
                // Asynchronously write the data to the destination socket
                net::async_write(
                    dest,
                    net::buffer(buffer->data(), bytes_transferred),
                    [&source, &dest, &buffer, self](boost::system::error_code ec, std::size_t /*length*/)
                    {
                      if (!ec)
                      {
                        // After writing data, keep the relay running
                        self->do_relay(source, dest, buffer);
                      }
                      else
                      {
                        // eof is normal
                        if (ec != boost::asio::error::eof)
                        {
                          std::cout << "Error writing to destination socket: " << ec.message() << std::endl;
                          dest.close(); // Close the destination socket on error
                        }
                      }
                    });
              }
              else
              {
                // oef is normal
                if (ec != boost::asio::error::eof)
                {
                  std::cout << "Error reading from source socket: " << ec.message() << std::endl;
                  dest.close(); // Close the destination socket on error
                }
              }
            }));
  }

  void write_unconsumed()
  {
    // Asynchronously write the data to the destination socket
    net::async_write(
        remote_socket_,
        buffer_,
        // net::buffer(client_to_server_buffer->data(), buffer_.size()),
        [self = shared_from_this()](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (ec)
          {
            std::cerr << "Error writing to proxy server: " << ec.message() << std::endl;
          }
          else
          {
            std::cout << "starting relay...." << std::endl;
            self->do_relay(self->socket_, self->remote_socket_, self->client_to_server_buffer);
            self->do_relay(self->remote_socket_, self->socket_, self->server_to_client_buffer);
          }
        });
  }
  // http::request<http::empty_body> req = parser_->release();
  // http::request_serializer<http::empty_body> sr{req};
  void send_parsed_request()
  {
    req_.erase(http::field::proxy_connection);
    std::ostringstream oss;
    beast::error_code ec;
    server_async::write_ostream(oss, req_, ec);
    http::request_serializer<http::empty_body> sr{req_};
    sr.split(true);

    std::cout << "request: " << req_ << std::endl;
    // http::async_write(socket_, net::buffer(oss.str()),
    http::async_write(remote_socket_, req_,
                      [self = shared_from_this()](boost::system::error_code ec, std::size_t)
                      {
                        if (!ec)
                        {
                          std::cout << "write request done." << std::endl;
                          self->write_unconsumed();
                        }
                        else
                        {
                          std::cerr << "Error writing to proxy server: " << ec.message() << std::endl;
                        }
                      });
  }

  void start()
  {
    boost::urls::url_view url{req_.target()};
    // tcp::resolver resolver_(socket_.get_executor());
    // auto delimiter_pos = target_endpoints_.find(':');

    std::string host = url.host();
    std::string port = url.port().empty() ? "80" : url.port();

    req_.set(http::field::host, host);
    // req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    // req_.target(path); // Set the target path
    // Optionally, you can handle the query if needed
    if (!url.query().empty())
    {
      req_.target(url.path() + "?" + url.query()); // Append query if present
    }
    else
    {
      req_.target(url.path());
    }

    std::cout << "start resolve: " << host << ", port: " << port << "version: " << req_.version() << std::endl;
    // tcp::resolver::results_type target_endpoints_resolved_ = resolver_.resolve(host, port);
    // std::cout << "result: " << target_endpoints_resolved_.begin()->endpoint() << std::endl;
    resolver_.async_resolve(host, port,
                            [self = shared_from_this()](boost::system::error_code ec, tcp::resolver::results_type results)
                            {
                              if (!ec)
                              {
                                std::cout << "result: " << results.begin()->endpoint() << std::endl;
                                boost::asio::async_connect(self->remote_socket_, results,
                                                           [self](boost::system::error_code ec, const tcp::endpoint &)
                                                           {
                                                             if (!ec)
                                                             {
                                                               self->send_parsed_request();
                                                             }
                                                             else
                                                             {
                                                               std::cerr << "Error connecting to remote server: " << ec.message() << std::endl;
                                                               self->cleanup();
                                                             }
                                                           });
                              }
                              else
                              {
                                std::cerr << "Error resolving target endpoints: " << ec.message() << ", value: " << ec.value() << std::endl;
                                self->cleanup();
                              }
                            });
  };

protected:
  beast::flat_buffer buffer_;
  http::request<http::empty_body> req_;

private:
  http_copier(
      tcp::socket &&socket_,
      http::request<http::empty_body> &&req_,
      beast::flat_buffer &&buffer_)
      : socket_(std::move(socket_)),
        req_(req_),
        buffer_(buffer_),
        remote_socket_(socket_.get_executor()),
        resolver_(socket_.get_executor()),
        client_to_server_buffer(std::make_shared<std::array<char, max_length>>()),
        server_to_client_buffer(std::make_shared<std::array<char, max_length>>())
  {
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
  tcp::resolver resolver_; // need keep live for who async operations.
  enum
  {
    max_length = 4096
  };
  std::shared_ptr<std::array<char, max_length>> client_to_server_buffer;
  std::shared_ptr<std::array<char, max_length>> server_to_client_buffer;
};
#endif
