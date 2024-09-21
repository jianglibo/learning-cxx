#pragma once

#ifndef HTTP_COPY_H
#define HTTP_COPY_H

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <optional>
#include <boost/url.hpp>
#include "socket_copier.h"

using boost::asio::ip::tcp;

namespace server_async
{

  template <typename StreamType, typename Derived, size_t max_length = 4096>
  class http_copier
  {
  public:
    // using pointer = std::shared_ptr<http_copier<StreamType>>;
    // static pointer create(
    //     StreamType &&socket_,
    //     http::request<http::empty_body> &&req_,
    //     beast::flat_buffer &&buffer_)
    // {
    //   return pointer(new http_copier(std::forward<StreamType>(socket_),
    //                                  std::forward<http::request<http::empty_body>>(req_),
    //                                  std::forward<beast::flat_buffer>(buffer_)));
    // }
    http_copier(
        StreamType &&stream_,
        http::request<http::empty_body> &&req_,
        beast::flat_buffer &&buffer_)
        : stream_(std::move(stream_)),
          req_(std::move(req_)),
          buffer_(std::move(buffer_)),
          remote_socket_(stream_.get_executor()),
          resolver_(stream_.get_executor()),
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
              std::cout << "starting relay...." << std::endl;
              std::cout << "start fetching....." << std::endl;
              do_relay(self, self->remote_socket_, self->stream_, self->to_client_buffer, false);
              std::cout << "start sending......" << std::endl;
              do_relay(self, self->stream_, self->remote_socket_, self->to_remote_buffer, true);
            }
          });
    }
    void send_parsed_request()
    {
      // req_.erase(http::field::proxy_connection);
      // std::ostringstream oss;
      // beast::error_code ec;
      // server_async::write_ostream(oss, req_, ec);
      // http::request_serializer<http::empty_body> sr{req_};
      // sr.split(true);
      std::cout << "request: " << req_ << std::endl;
      http::async_write(remote_socket_, req_,
                        [self = derived().shared_from_this()](boost::system::error_code ec, std::size_t)
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
      std::cout << "start........" << std::endl;
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
                                                                 self->send_parsed_request();
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
    http::request<http::empty_body> req_;
    StreamType stream_;
    tcp::socket remote_socket_;

  private:
    tcp::resolver resolver_; // need keep live for who async operations.
    std::shared_ptr<std::array<char, max_length>> to_remote_buffer;
    std::shared_ptr<std::array<char, max_length>> to_client_buffer;
  };

  class plain_http_copy : public http_copier<beast::tcp_stream, plain_http_copy>,
                          public std::enable_shared_from_this<plain_http_copy>
  {
  public:
    plain_http_copy(
        beast::tcp_stream &&stream_,
        http::request<http::empty_body> &&req_,
        beast::flat_buffer &&buffer_)
        : http_copier<beast::tcp_stream, plain_http_copy>(
              std::move(stream_),
              std::move(req_),
              std::move(buffer_))
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

  class ssl_http_copy : public http_copier<ssl_beast_stream, ssl_http_copy>,
                        public std::enable_shared_from_this<ssl_http_copy>
  {
  public:
    ssl_http_copy(
        ssl_beast_stream &&stream_,
        http::request<http::empty_body> &&req_,
        beast::flat_buffer &&buffer_)
        : http_copier<ssl_beast_stream, ssl_http_copy>(
              std::move(stream_),
              std::move(req_),
              std::move(buffer_))
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
              &ssl_http_copy::on_shutdown,
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
