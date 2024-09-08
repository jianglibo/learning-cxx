#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <iostream>
#include <memory>
#include <thread>

using boost::asio::ip::tcp;

class SSHServer : public std::enable_shared_from_this<SSHServer>
{
public:
	SSHServer(boost::asio::io_context &io_context, short port, boost::asio::ssl::context &ssl_context)
		: acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
		  socket_(io_context),
		  ssl_context_(ssl_context)
	{
		start_accept();
	}

private:
	void start_accept()
	{
		acceptor_.async_accept(socket_, [this](boost::system::error_code ec)
							   {
								   if (!ec)
								   {
									   // Create a new SSL socket and perform the handshake
									   auto ssl_socket = std::make_shared<boost::asio::ssl::stream<tcp::socket>>(std::move(socket_), ssl_context_);
									   ssl_socket->async_handshake(boost::asio::ssl::stream_base::server,
																   [this, ssl_socket](boost::system::error_code ec)
																   {
																	   if (!ec)
																	   {
																		   start_read(ssl_socket); // Start reading data from the client
																	   }
																	   else
																	   {
																		   std::cerr << "Error during handshake: " << ec.message() << std::endl;
																	   }
																   });
								   }
								   else
								   {
									   std::cerr << "Error accepting connection: " << ec.message() << std::endl;
								   }
								   start_accept(); // Accept other connections
							   });
	}

	void start_read(std::shared_ptr<boost::asio::ssl::stream<tcp::socket>> ssl_socket)
	{
		auto self(shared_from_this());
		boost::asio::async_read_until(*ssl_socket, buffer_, '\n',
									  [this, ssl_socket](boost::system::error_code ec, std::size_t length)
									  {
										  if (!ec)
										  {
											  std::istream is(&buffer_);
											  std::string message;
											  std::getline(is, message);
											  std::cout << "Received: " << message << std::endl;

											  if (message == "login")
											  {
												  authenticate_client(ssl_socket);
											  }
											  else if (message == "forward")
											  {
												  setup_port_forwarding(ssl_socket);
											  }
										  }
									  });
	}

	void authenticate_client(std::shared_ptr<boost::asio::ssl::stream<tcp::socket>> ssl_socket)
	{
		// Simplified authentication logic, e.g., just acknowledge login
		boost::asio::async_write(*ssl_socket, boost::asio::buffer("Authenticated\n"),
								 [ssl_socket](boost::system::error_code ec, std::size_t)
								 {
									 if (!ec)
									 {
										 std::cout << "Client authenticated!" << std::endl;
										 // After successful authentication, the server is ready for more actions like port forwarding.
									 }
								 });
	}

	void setup_port_forwarding(std::shared_ptr<boost::asio::ssl::stream<tcp::socket>> ssl_socket)
	{
		// Here you would implement port forwarding logic
		std::cout << "Port forwarding setup..." << std::endl;
		// You would handle the port forwarding here, similar to the snippet in the previous message.
	}

	tcp::acceptor acceptor_;
	tcp::socket socket_;
	boost::asio::ssl::context &ssl_context_;
	boost::asio::streambuf buffer_;
};

// Main function to initialize the server
int main(int argc, char *argv[])
{
	try
	{
		if (argc != 2)
		{
			std::cerr << "Usage: ssh_server <port>" << std::endl;
			return 1;
		}

		boost::asio::io_context io_context;

		// SSL context for the server (using TLSv1.2)
		boost::asio::ssl::context ssl_context(boost::asio::ssl::context::sslv23);
		ssl_context.set_options(boost::asio::ssl::context::default_workarounds |
								boost::asio::ssl::context::no_sslv2 |
								boost::asio::ssl::context::single_dh_use);

		// crt file name is from environment variable MY_SSH_SERVER_CRT and key file name is from environment variable MY_SSH_SERVER_KEY
		//  Load server certificate and private key (make sure you have your certificate and private key ready)
		std::string crt_file = std::getenv("MY_SSH_SERVER_CRT");
		std::string key_file = std::getenv("MY_SSH_SERVER_KEY");
		std::cout << "Using certificate file: " << crt_file << std::endl;
		std::cout << "Using key file: " << key_file << std::endl;

		ssl_context.use_certificate_chain_file(crt_file);
		ssl_context.use_private_key_file(key_file, boost::asio::ssl::context::pem);

		// Start the SSH server
		// std::make_shared<SSHServer>(io_context, std::atoi(argv[1]), ssl_context)->start_accept();
		std::shared_ptr<SSHServer> server = std::make_shared<SSHServer>(io_context, std::atoi(argv[1]), ssl_context);

		// Run the server
		io_context.run();
	}
	catch (std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}

	return 0;
}
