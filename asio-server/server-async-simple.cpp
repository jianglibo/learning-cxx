#include "server-async-simple.h"

// tcp::socket &socket() { return socket_; }

void tcp_connection::start()
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
}

std::string tcp_connection::parse_http_request(const std::string &request)
{
  std::istringstream request_stream(request);
  std::string method, url, version;
  std::string line;
  std::stringstream new_request;

  // Read the first line (request line)
  if (std::getline(request_stream, line))
  {
    std::istringstream line_stream(line);
    line_stream >> method >> url >> version;
    std::cout << line << std::endl;

    // // If it's an HTTP proxy request (contains a full URL starting with "http://")
    // if (url.substr(0, 7) == "http://")
    // {
    //   auto pos = url.find('/', 7); // Find the first '/' after "http://"
    //   if (pos != std::string::npos)
    //   {
    //     url = url.substr(pos); // Strip the "http://hostname" part, keep only the path
    //   }
    // }

    // Construct the new request line with the modified URL (if needed)
    new_request << method << " " << url << " " << version << "\r\n";
  }

  // Add the remaining headers and body
  while (std::getline(request_stream, line))
  {
    new_request << line << "\r\n";
    std::cout << "Header: " << line << std::endl;
  }
  // Add a blank line to indicate the end of the headers
  new_request << "\r\n";
  std::cout << "parse header done." << std::endl;
  return new_request.str();
}

void tcp_connection::do_read_client()
{
  auto self(shared_from_this());
  // std::cout << "do_read_client get called." << std::endl;
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
                            if (debug_mode)
                            {
                              if (header_state_ == HeaderState::READING_HEADERS)
                              {
                                // Accumulate the received data into the request buffer
                                request_buffer_.append(data_client_, length);
                                // std::cout << "Data read from client, length: " << length << std::endl;
                                // Check if we have received a full HTTP request (i.e., headers end with \r\n\r\n)
                                if (has_complete_http_request(request_buffer_))
                                {
                                  // Process the full HTTP request
                                  request_buffer_ = parse_http_request(request_buffer_);
                                  // Clear the buffer and continue processing the request
                                  // request_buffer_.clear();
                                  // if modified_request.size() > max_length, we need to send it in chunks
                                  // std::size_t total_length = modified_request.size();
                                  // Loop to send the request in chunks if it's larger than max_length
                                  // std::size_t chunk_size = std::min(static_cast<size_t>(max_length), total_length);
                                  // std::memcpy(data_client_, modified_request.data(), chunk_size);
                                  // waiting_to_proxy_ = std::max(total_length - chunk_size, (size_t)0);
                                  // when invoking do_write_proxy, do_read_client will not be called till do_write_proxy is done.
                                  do_write_proxy(0);
                                }
                                else
                                {
                                  // Not a complete request yet, continue reading
                                  do_read_client();
                                }
                              }
                              else if (header_state_ == HeaderState::READING_BODY)
                              {
                                do_write_proxy(length);
                              }
                            }
                            else
                            {
                              do_write_proxy(length);
                            }
                          });
}

void tcp_connection::do_write_proxy(std::size_t length)
{
  auto self(shared_from_this());
  if (length == 0)
  {
    if (!request_buffer_.empty())
    {
      boost::asio::async_write(remote_socket_, boost::asio::buffer(request_buffer_),
                               [this, self](boost::system::error_code ec, std::size_t)
                               {
                                 if (!ec)
                                 {
                                   request_buffer_.clear();
                                   do_read_client();
                                 }
                                 else
                                 {
                                   std::cerr << "Error writing to proxy server: " << ec.message() << std::endl;
                                   cleanup();
                                 }
                               });
    }
    else
    {
      do_read_client();
    }
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

void tcp_connection::do_read_proxy()
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

void tcp_connection::do_write_client(std::size_t length)
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

void tcp_server::start_accept()
{
  // auto new_connection = tcp_connection::create(io_context_, proxy_address_, proxy_port_, debug_mode);
  auto new_connection = tcp_connection::create(io_context_, proxy_endpoints_, debug_mode);
  acceptor_.async_accept(new_connection->socket(),
                         [this, new_connection](boost::system::error_code ec)
                         {
                           if (!ec)
                           {
                             //  std::cout << "Accepted new connection" << std::endl;
                             new_connection->start();
                             start_accept();
                           }
                           else
                           {
                             std::cerr << "Error accepting connection: " << ec.message() << std::endl;
                           }
                         });
}
