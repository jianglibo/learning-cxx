#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

using boost::asio::ip::tcp;

class tcp_connection : public std::enable_shared_from_this<tcp_connection>
{
public:
  using pointer = std::shared_ptr<tcp_connection>;

  static pointer create(boost::asio::io_context &io_context,
                        const std::string &proxy_address, const std::string &proxy_port)
  {
    return pointer(new tcp_connection(io_context, proxy_address, proxy_port));
  }

  tcp::socket &socket() { return socket_; }

  void start()
  {
    do_read();
  }

private:
  tcp_connection(boost::asio::io_context &io_context,
                 const std::string &proxy_address, const std::string &proxy_port)
      : socket_(io_context),
        remote_socket_(io_context),
        resolver_(io_context),
        proxy_address_(proxy_address),
        proxy_port_(proxy_port)
  {
  }

  void do_read()
  {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
                            [this, self](boost::system::error_code ec, std::size_t length)
                            {
                              if (ec)
                              {
                                if (ec == boost::asio::error::eof)
                                {
                                  // Connection closed cleanly by the client
                                  std::cout << "Client closed connection" << std::endl;
                                }
                                else
                                {
                                  std::cerr << "Error reading from client: " << ec.message() << std::endl;
                                }
                                cleanup();
                                return;
                              }
                              std::cout << "Data read from client, length: " << length << std::endl;
                              do_forward(length);
                            });
  }

  void do_forward(std::size_t length)
  {
    auto self(shared_from_this());
    if (!remote_socket_.is_open())
    {
      resolver_.async_resolve(proxy_address_, proxy_port_,
                              [this, self, length](boost::system::error_code ec, tcp::resolver::results_type endpoints)
                              {
                                if (!ec)
                                {
                                  boost::asio::async_connect(remote_socket_, endpoints,
                                                             [this, self, length](boost::system::error_code ec, const tcp::endpoint &)
                                                             {
                                                               if (!ec)
                                                               {
                                                                 std::cout << "Connected to proxy server" << std::endl;
                                                                 do_write(length);
                                                               }
                                                               else
                                                               {
                                                                 std::cerr << "Error connecting to proxy server: " << ec.message() << std::endl;
                                                                 cleanup();
                                                               }
                                                             });
                                }
                                else
                                {
                                  std::cerr << "Error resolving proxy address: " << ec.message() << std::endl;
                                  cleanup();
                                }
                              });
    }
    else
    {
      do_write(length);
    }
  }

  void do_write(std::size_t length)
  {
    auto self(shared_from_this());
    boost::asio::async_write(remote_socket_, boost::asio::buffer(data_, length),
                             [this, self](boost::system::error_code ec, std::size_t)
                             {
                               if (!ec)
                               {
                                 std::cout << "Data written to proxy server" << std::endl;
                                 do_read_remote();
                               }
                               else
                               {
                                 std::cerr << "Error writing to proxy server: " << ec.message() << std::endl;
                                 cleanup();
                               }
                             });
  }

  void do_read_remote()
  {
    auto self(shared_from_this());
    remote_socket_.async_read_some(boost::asio::buffer(data_, max_length),
                                   [this, self](boost::system::error_code ec, std::size_t length)
                                   {
                                     if (ec)
                                     {
                                       if (ec == boost::asio::error::eof)
                                       {
                                         std::cout << "Proxy server closed connection" << std::endl;
                                       }
                                       else
                                       {
                                         std::cerr << "Error reading from proxy server: " << ec.message() << std::endl;
                                       }
                                       cleanup();
                                       return;
                                     }
                                     std::cout << "Data read from proxy server, length: " << length << std::endl;
                                     do_write_client(length);
                                   });
  }

  void do_write_client(std::size_t length)
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
                             [this, self](boost::system::error_code ec, std::size_t)
                             {
                               if (!ec)
                               {
                                 std::cout << "Data written to client" << std::endl;
                                 do_read();
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
  std::string proxy_address_;
  std::string proxy_port_;
  enum
  {
    max_length = 4096
  };
  char data_[max_length];
};

class tcp_server
{
public:
  tcp_server(boost::asio::io_context &io_context, const std::string &proxy_address, const std::string &proxy_port)
      : io_context_(io_context),
        acceptor_(io_context, tcp::endpoint(tcp::v4(), 7890)),
        proxy_address_(proxy_address),
        proxy_port_(proxy_port)
  {
    start_accept();
  }

private:
  void start_accept()
  {
    auto new_connection = tcp_connection::create(io_context_, proxy_address_, proxy_port_);

    acceptor_.async_accept(new_connection->socket(),
                           [this, new_connection](boost::system::error_code ec)
                           {
                             if (!ec)
                             {
                               std::cout << "Accepted new connection" << std::endl;
                               new_connection->start();
                             }
                             else
                             {
                               std::cerr << "Error accepting connection: " << ec.message() << std::endl;
                             }
                             start_accept();
                           });
  }

  boost::asio::io_context &io_context_;
  tcp::acceptor acceptor_;
  std::string proxy_address_;
  std::string proxy_port_;
};

int main()
{
  try
  {
    boost::asio::io_context io_context;
    tcp_server server(io_context, "192.168.0.90", "8000");
    io_context.run();
  }
  catch (std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << std::endl;
  }

  return 0;
}

// #include <iostream>
// #include <memory>
// #include <utility>
// #include <boost/asio.hpp>

// using boost::asio::ip::tcp;

// class tcp_connection : public std::enable_shared_from_this<tcp_connection>
// {
// public:
//   typedef std::shared_ptr<tcp_connection> pointer;

//   static pointer create(boost::asio::io_context &io_context, const std::string &proxy_address, const std::string &proxy_port)
//   {
//     return pointer(new tcp_connection(io_context, proxy_address, proxy_port));
//   }

//   tcp::socket &socket()
//   {
//     return socket_;
//   }

//   void start()
//   {
//     // Start the async read operation from the client socket
//     do_read();
//   }

// private:
//   tcp_connection(boost::asio::io_context &io_context, const std::string &proxy_address, const std::string &proxy_port)
//       : socket_(io_context), remote_socket_(io_context), proxy_address_(proxy_address), proxy_port_(proxy_port), resolver_(io_context)
//   {
//   }

//   void do_read()
//   {
//     auto self(shared_from_this());
//     socket_.async_read_some(boost::asio::buffer(data_, max_length),
//                             [this, self](boost::system::error_code ec, std::size_t length)
//                             {
//                               if (!ec)
//                               {
//                                 // Forward data to the remote server (proxy server)
//                                 do_forward(length);
//                               } else {
//                                 std::cerr << "Read error: " << ec.message() << std::endl;
//                               }
//                             });
//   }

//   void do_forward(std::size_t length)
//   {
//     auto self(shared_from_this());
//     if (!remote_socket_.is_open())
//     {
//       // Connect to the proxy server directly
//       tcp::resolver::query query(proxy_address_, proxy_port_);
//       auto endpoints = resolver_.resolve(query);
//       std::cout << "Connecting to proxy server " << proxy_address_ << ":" << proxy_port_ << std::endl;

//       boost::asio::async_connect(remote_socket_, endpoints,
//                                  [this, self, length](boost::system::error_code ec, const tcp::endpoint &)
//                                  {
//                                    if (!ec)
//                                    {
//                                      do_write(length);
//                                    }
//                                    else
//                                    {
//                                      std::cerr << "Connect error: " << ec.message() << std::endl;
//                                    }
//                                  });
//     }
//     else
//     {
//       do_write(length);
//     }
//   }

//   void do_write(std::size_t length)
//   {
//     auto self(shared_from_this());
//     boost::asio::async_write(remote_socket_, boost::asio::buffer(data_, length),
//                              [this, self](boost::system::error_code ec, std::size_t)
//                              {
//                                if (!ec)
//                                {
//                                  // Read response from remote server (proxy server)
//                                  do_read_remote();
//                                } else {
//                                   std::cerr << "Write error: " << ec.message() << std::endl;
//                                }
//                              });
//   }

//   void do_read_remote()
//   {
//     auto self(shared_from_this());
//     remote_socket_.async_read_some(boost::asio::buffer(data_, max_length),
//                                    [this, self](boost::system::error_code ec, std::size_t length)
//                                    {
//                                      if (!ec)
//                                      {
//                                        // Forward response back to the client
//                                        do_write_client(length);
//                                      } else {
//                                         std::cerr << "Read error: " << ec.message() << std::endl;
//                                      }
//                                    });
//   }

//   void do_write_client(std::size_t length)
//   {
//     auto self(shared_from_this());
//     boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
//                              [this, self](boost::system::error_code ec, std::size_t)
//                              {
//                                if (!ec)
//                                {
//                                  // Continue reading from the client
//                                  do_read();
//                                } else {
//                                   std::cerr << "Write error: " << ec.message() << std::endl;
//                                }
//                              });
//   }

//   tcp::socket socket_;        // Client socket
//   tcp::socket remote_socket_; // Socket for connecting to the proxy server
//   tcp::resolver resolver_;    // Resolver for the proxy server
//   std::string proxy_address_; // Proxy server address
//   std::string proxy_port_;    // Proxy server port
//   enum
//   {
//     max_length = 1024
//   };
//   char data_[max_length];
// };

// class tcp_server
// {
// public:
//   tcp_server(boost::asio::io_context &io_context, const std::string &proxy_address, const std::string &proxy_port)
//       : io_context_(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), 7890)),
//         proxy_address_(proxy_address), proxy_port_(proxy_port)
//   {
//     start_accept();
//   }

// private:
//   void start_accept()
//   {
//     tcp_connection::pointer new_connection =
//         tcp_connection::create(io_context_, proxy_address_, proxy_port_);

//     acceptor_.async_accept(new_connection->socket(),
//                            [this, new_connection](boost::system::error_code ec)
//                            {
//                              if (!ec)
//                              {
//                                new_connection->start();
//                              }
//                              else
//                              {
//                                std::cerr << "Accept error: " << ec.message() << std::endl;
//                              }
//                              start_accept();
//                            });
//   }

//   boost::asio::io_context &io_context_;
//   tcp::acceptor acceptor_;
//   std::string proxy_address_; // Proxy server address
//   std::string proxy_port_;    // Proxy server port
// };

// int main()
// {
//   try
//   {
//     boost::asio::io_context io_context;
//     tcp_server server(io_context, "192.168.0.90", "8000");
//     io_context.run();
//   }
//   catch (std::exception &e)
//   {
//     std::cerr << "Exception: " << e.what() << std::endl;
//   }

//   return 0;
// }
