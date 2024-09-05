#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class tcp_connection
    : public std::enable_shared_from_this<tcp_connection>
{
public:
  typedef std::shared_ptr<tcp_connection> pointer;

  static pointer create(boost::asio::io_context &io_context,
                        const tcp::resolver::results_type &endpoints)
  {
    return pointer(new tcp_connection(io_context, endpoints));
  }

  tcp::socket &socket()
  {
    return socket_;
  }

  void start()
  {
    // Start the async read operation from the client socket
    do_read();
  }

private:
  tcp_connection(boost::asio::io_context &io_context,
                 const tcp::resolver::results_type &endpoints)
      : socket_(io_context),
        remote_socket_(io_context),
        resolver_(io_context),
        endpoints_(endpoints)
  {
  }

  void do_read()
  {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
                            [this, self](boost::system::error_code ec, std::size_t length)
                            {
                              if (!ec)
                              {
                                // Forward data to remote server
                                do_forward(length);
                              }
                            });
  }

  void do_forward(std::size_t length)
  {
    auto self(shared_from_this());
    if (!remote_socket_.is_open())
    {
      // Resolve the remote server address and connect
      resolver_.async_resolve("remote_server_address", "port",
                              [this, self, length](boost::system::error_code ec, tcp::resolver::results_type endpoints)
                              {
                                if (!ec)
                                {
                                  boost::asio::async_connect(remote_socket_, endpoints,
                                                             [this, self, length](boost::system::error_code ec, const tcp::endpoint &)
                                                             {
                                                               if (!ec)
                                                               {
                                                                 do_write(length);
                                                               }
                                                             });
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
                                 // Read response from remote server
                                 do_read_remote();
                               }
                             });
  }

  void do_read_remote()
  {
    auto self(shared_from_this());
    remote_socket_.async_read_some(boost::asio::buffer(data_, max_length),
                                   [this, self](boost::system::error_code ec, std::size_t length)
                                   {
                                     if (!ec)
                                     {
                                       // Forward response back to the client
                                       do_write_client(length);
                                     }
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
                                 // Continue reading from the client
                                 do_read();
                               }
                             });
  }

  tcp::socket socket_;
  tcp::socket remote_socket_;
  tcp::resolver resolver_;
  tcp::resolver::results_type endpoints_;
  enum
  {
    max_length = 1024
  };
  char data_[max_length];
};

class tcp_server
{
public:
  tcp_server(boost::asio::io_context &io_context)
      : io_context_(io_context),
        acceptor_(io_context, tcp::endpoint(tcp::v4(), 13)),
        resolver_(io_context)
  {
    start_accept();
  }

private:
  void start_accept()
  {
    tcp_connection::pointer new_connection =
        tcp_connection::create(io_context_, resolver_.resolve("remote_server_address", "port"));

    acceptor_.async_accept(new_connection->socket(),
                           [this, new_connection](boost::system::error_code ec)
                           {
                             if (!ec)
                             {
                               new_connection->start();
                             }
                             start_accept();
                           });
  }

  boost::asio::io_context &io_context_;
  tcp::acceptor acceptor_;
  tcp::resolver resolver_;
};

int main()
{
  try
  {
    boost::asio::io_context io_context;
    tcp_server server(io_context);
    io_context.run();
  }
  catch (std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << std::endl;
  }

  return 0;
}
