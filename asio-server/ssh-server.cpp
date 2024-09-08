#include <boost/asio.hpp>
#include <libssh2.h>
#include <iostream>
#include <memory>

using boost::asio::ip::tcp;

class SSHServer : public std::enable_shared_from_this<SSHServer>
{
public:
	SSHServer(boost::asio::io_context &io_context, short port)
		: acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
		  socket_(io_context)
	{
		libssh2_init(0); // Initialize libssh2
		start_accept();
	}

	~SSHServer()
	{
		libssh2_exit(); // Cleanup libssh2
	}

private:
	void start_accept()
	{
		acceptor_.async_accept(socket_, [this](boost::system::error_code ec)
							   {
								   if (!ec)
								   {
									   handle_ssh_connection(std::move(socket_));
								   }
								   else
								   {
									   std::cerr << "Error accepting connection: " << ec.message() << std::endl;
								   }
								   start_accept(); // Accept other connections
							   });
	}

	void handle_ssh_connection(tcp::socket socket)
	{
		std::shared_ptr<tcp::socket> sock_ptr = std::make_shared<tcp::socket>(std::move(socket));

		// Create a new SSH session
		LIBSSH2_SESSION *session = libssh2_session_init();
		if (!session)
		{
			std::cerr << "Failed to initialize SSH session." << std::endl;
			return;
		}

		// Perform the handshake
		int rc = libssh2_session_handshake(session, sock_ptr->native_handle());
		if (rc)
		{
			std::cerr << "Error during SSH handshake: " << rc << std::endl;
			libssh2_session_free(session);
			return;
		}

		// Authentication (simplified for demonstration)
		const char *username = "test";
		const char *password = "password";

		rc = libssh2_userauth_password(session, username, password);
		if (rc)
		{
			std::cerr << "Authentication failed." << std::endl;
		}
		else
		{
			std::cout << "Client authenticated successfully!" << std::endl;
			// You can now handle SSH commands, shells, etc.
		}

		libssh2_session_disconnect(session, "Bye");
		libssh2_session_free(session);
	}

	tcp::acceptor acceptor_;
	tcp::socket socket_;
};

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
		SSHServer server(io_context, std::atoi(argv[1]));
		io_context.run();
	}
	catch (std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}

	return 0;
}
