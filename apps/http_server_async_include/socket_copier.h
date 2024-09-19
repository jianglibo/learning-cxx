#pragma once

#ifndef SOCKET_COPY_H
#define SOCKET_COPY_H

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <optional>
#include <boost/url.hpp>

using boost::asio::ip::tcp;

class socket_copier : public std::enable_shared_from_this<socket_copier>
{
public:
  using pointer = std::shared_ptr<socket_copier>;
  static pointer create(
      tcp::socket &&socket_,
      const std::string &target_endpoints)
  {
    return pointer(new socket_copier(std::forward<tcp::socket>(socket_), target_endpoints));
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

  void write200ok(const std::string &response)
  {
    boost::asio::async_write(socket_, boost::asio::buffer(response),
                             [self = shared_from_this()](boost::system::error_code ec, std::size_t)
                             {
                               if (!ec)
                               {
                                 std::cout << "starting relay...." << std::endl;
                                 self->do_relay(self->socket_, self->remote_socket_, self->client_to_server_buffer);
                                 self->do_relay(self->remote_socket_, self->socket_, self->server_to_client_buffer);
                               }
                               else
                               {
                                 std::cerr << "Error writing to proxy server: " << ec.message() << std::endl;
                               }
                             });
  }

  void start()
  {
    // boost::urls::url_view url{target_endpoints_};
    // tcp::resolver resolver_(socket_.get_executor());
    auto delimiter_pos = target_endpoints_.find(':');

    const std::string &host = target_endpoints_.substr(0, delimiter_pos);
    const std::string &port = target_endpoints_.substr(delimiter_pos + 1);

    std::cout << "target_endpoints_:" << target_endpoints_ << ", start resolve: " << host << ", port: " << port << std::endl;
    tcp::resolver::results_type target_endpoints_resolved_ = resolver_.resolve(host, port);
    std::cout << "result: " << target_endpoints_resolved_.begin()->endpoint() << std::endl;
    resolver_.async_resolve(host, port,
                            [self = shared_from_this()](boost::system::error_code ec, tcp::resolver::results_type results)
                            {
                              if (!ec)
                              {
                                boost::asio::async_connect(self->remote_socket_, results,
                                                           [self](boost::system::error_code ec, const tcp::endpoint &)
                                                           {
                                                             if (!ec)
                                                             {
                                                               self->write200ok("HTTP/1.1 200 OK\r\nConnection: keep-alive\r\n\r\n");
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

private:
  socket_copier(
      tcp::socket &&socket_,
      const std::string &target_endpoints)
      : socket_(std::move(socket_)),
        remote_socket_(socket_.get_executor()),
        resolver_(socket_.get_executor()),
        target_endpoints_(target_endpoints),
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
  const std::string &target_endpoints_;
  enum
  {
    max_length = 4096
  };
  std::shared_ptr<std::array<char, max_length>> client_to_server_buffer;
  std::shared_ptr<std::array<char, max_length>> server_to_client_buffer;
};
#endif
