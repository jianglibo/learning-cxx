#pragma once

#ifndef SOCKET_COPY_H
#define SOCKET_COPY_H

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <optional>
#include <boost/url.hpp>

#include "server_async_util.h"

using boost::asio::ip::tcp;

namespace server_async
{
  using ssl_beast_stream = ssl::stream<beast::tcp_stream>;

  template <typename T>
  class has_do_eof
  {
  private:
    template <typename U>
    static auto test(int) -> decltype(std::declval<U>().do_eof(), std::true_type()) {};

    template <typename>
    static std::false_type test(...)
    {
      return std::false_type();
    };

  public:
    static constexpr bool value = decltype(test<T>(0))::value;
  };
  template <typename SourceType, typename DestType, typename Derived, size_t max_length = 4096,
            typename = std::enable_if_t<has_do_eof<Derived>::value>>
  void do_relay(std::shared_ptr<Derived> self, SourceType &source, DestType &dest, std::shared_ptr<std::array<char, max_length>> &buffer, bool to_remote)
  {
    source.async_read_some(
        boost::asio::buffer(*buffer),
        boost::asio::bind_executor(
            source.get_executor(),
            [self, &source, &dest, &buffer, &to_remote](boost::system::error_code ec, std::size_t bytes_transferred)
            {
              if (!ec)
              {
                boost::asio::async_write(
                    dest,
                    boost::asio::buffer(buffer->data(), bytes_transferred),
                    [self, &source, &dest, &buffer, &to_remote](boost::system::error_code ec, std::size_t)
                    {
                      if (!ec)
                      {
                        do_relay(self, source, dest, buffer, to_remote); // Continue relaying
                      }
                      else
                      {
                        std::cout << "Error during write to: " << typeid(DestType).name() << ", msg: " << ec.message() << std::endl;
                        self->do_eof();
                      }
                    });
              }
              else
              {
                if (ec != boost::asio::error::eof)
                {
                  std::cout << "Error during read from: " << typeid(SourceType).name() << ", msg: " << ec.message() << std::endl;
                }
                self->do_eof();
              }
            }));
  }

  template <typename StreamType, typename Derived, size_t max_length = 4096>
  class socket_copier
  {
  public:
    // using pointer = std::shared_ptr<socket_copier>;

    // static pointer create(
    //     tcp::socket &&stream_,
    //     beast::flat_buffer &&buffer_,
    //     const std::string &target_endpoints)
    // {
    //   return std::make_shared<pointer>(std::forward<tcp::socket>(stream_),
    //                                    std::forward<beast::flat_buffer>(buffer_),
    //                                    target_endpoints);
    // }
    socket_copier(
        StreamType &&stream_,
        beast::flat_buffer &&buffer_,
        http::request<http::empty_body> &&req_)
        : stream_(std::move(stream_)),
          remote_socket_(stream_.get_executor()),
          resolver_(stream_.get_executor()),
          req_(std::move(req_)),
          to_remote_buffer(std::make_shared<std::array<char, max_length>>()),
          to_client_buffer(std::make_shared<std::array<char, max_length>>())
    {
    }

    // Access the derived class, this is part of
    // the Curiously Recurring Template Pattern idiom.
    Derived &
    derived()
    {
      return static_cast<Derived &>(*this);
    }

    void log_error(const boost::system::error_code &ec, bool to_remote, const std::string &operation)
    {
      if (ec != boost::asio::error::eof)
      {
        if (to_remote)
        {
          std::cout << "Error " << operation << " to remote: " << ec.message() << std::endl;
        }
        else
        {
          std::cout << "Error " << operation << " to client: " << ec.message() << std::endl;
        }
      }
    }

    void write_unconsumed()
    {
      // Asynchronously write the data to the destination socket
      net::async_write(
          remote_socket_,
          buffer_,
          // net::buffer(client_to_server_buffer->data(), buffer_.size()),
          [self = derived().shared_from_this()](boost::system::error_code ec, std::size_t /*length*/)
          {
            if (ec)
            {
              std::cerr << "Error writing to proxy server: " << ec.message() << std::endl;
            }
            else
            {
              std::cout << "start fetching from remote and send to client....." << std::endl;
              do_relay(self, self->remote_socket_, self->stream_, self->to_client_buffer, false);
              std::cout << "start fetching from client and sending to remote......" << std::endl;
              // if (self->req_.method() == http::verb::post || self->req_.method() == http::verb::put)
              do_relay(self, self->stream_, self->remote_socket_, self->to_remote_buffer, true);
            }
          });
    }

    void write200ok_to_client(const std::string &response)
    {
      boost::asio::async_write(stream_, boost::asio::buffer(response),
                               [self = derived().shared_from_this()](boost::system::error_code ec, std::size_t)
                               {
                                 if (!ec)
                                 {
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
      // boost::urls::url_view url{req_.target()};
      // if target has schema
      // std::string tg;
      // std::string default_port;
      // if (req_.target().starts_with("https:"))
      // {
      //   tg = req_.target();
      //   default_port = "443";
      // }
      // else if (req_.target().starts_with("http:"))
      // {
      //   tg = req_.target();
      //   default_port = "80";
      // }
      // else
      // {
      //   tg = std::string{"//"} + req_.target().data();
      //   default_port = "80";
      // }

      // std::cout << "target_endpoints_:" << tg << std::endl;
      // auto ru = boost::urls::parse_uri_reference(tg);
      // std::cout << "parse result: " << ru.value() << std::endl;
      // if (ru.has_error())
      // {
      //   std::cout << "Error parsing target: " << ru.error().message() << std::endl;
      //   return;
      // }
      // // parse manually. example.com:443
      // const std::string &host = ru.value().host();
      // std::string port;
      // if (ru.value().has_port())
      // {
      //   port = ru.value().port();
      // }
      // else
      // {
      //   port = default_port;
      // }

      int deliminator = req_.target().find(':');
      if (deliminator == std::string::npos)
      {
        std::cerr << "Error parsing target: " << req_.target() << std::endl;
        return;
      }
      const std::string &host = req_.target().substr(0, deliminator);
      std::string port = req_.target().substr(deliminator + 1);

      std::cout << "target_endpoints_:" << req_.target() << ", start resolve: " << host << ", port: " << port << std::endl;
      // tcp::resolver::results_type target_endpoints_resolved_ = resolver_.resolve(host, port);
      resolver_.async_resolve(host, port,
                              [self = derived().shared_from_this()](boost::system::error_code ec, tcp::resolver::results_type results)
                              {
                                if (!ec)
                                {
                                  std::cout << "result: " << results.begin()->endpoint() << std::endl;
                                  boost::asio::async_connect(self->remote_socket_, results,
                                                             [self](boost::system::error_code ec, const tcp::endpoint &)
                                                             {
                                                               if (!ec)
                                                               {
                                                                 self->write200ok_to_client("HTTP/1.1 200 OK\r\nConnection: keep-alive\r\n\r\n");
                                                               }
                                                               else
                                                               {
                                                                 std::cerr << "Error connecting to remote server: " << ec.message() << std::endl;
                                                                 //  self->cleanup();
                                                               }
                                                             });
                                }
                                else
                                {
                                  std::cerr << "Error resolving target endpoints: " << ec.message() << ", value: " << ec.value() << std::endl;
                                  // self->cleanup();
                                }
                              });
    };

  protected:
    beast::flat_buffer buffer_;
    StreamType stream_;
    tcp::socket remote_socket_;

  private:
    tcp::resolver resolver_; // need keep live for who async operations.
    http::request<http::empty_body> req_;
    std::shared_ptr<std::array<char, max_length>> to_remote_buffer;
    std::shared_ptr<std::array<char, max_length>> to_client_buffer;
  };

  class plain_socket_copy : public socket_copier<beast::tcp_stream, plain_socket_copy>,
                            public std::enable_shared_from_this<plain_socket_copy>
  {
  public:
    plain_socket_copy(
        beast::tcp_stream &&stream_,
        beast::flat_buffer &&buffer_,
        http::request<http::empty_body> &&req_)
        : socket_copier<beast::tcp_stream, plain_socket_copy>(
              std::move(stream_),
              std::move(buffer_),
              std::move(req_))
    {
    }

    void
    do_eof()
    {
      // Send a TCP shutdown
      beast::error_code ec;
      stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
      // At this point the connection is closed gracefully
    }
  };

  class ssl_socket_copy : public socket_copier<ssl_beast_stream, ssl_socket_copy>,
                          public std::enable_shared_from_this<ssl_socket_copy>
  {
  public:
    ssl_socket_copy(
        ssl_beast_stream &&stream_,
        beast::flat_buffer &&buffer_,
        http::request<http::empty_body> &&req_)
        : socket_copier<ssl_beast_stream, ssl_socket_copy>(std::move(stream_),
                                                           std::move(buffer_),
                                                           std::move(req_))
    {
    }

    void
    do_eof()
    {
      // Set the timeout.
      beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

      // Perform the SSL shutdown
      stream_.async_shutdown(
          beast::bind_front_handler(
              &ssl_socket_copy::on_shutdown,
              shared_from_this()));
    }

    void
    on_shutdown(beast::error_code ec)
    {
      if (ec)
        return fail(ec, "shutdown");
      // At this point the connection is closed gracefully
    }
  };
}
#endif
