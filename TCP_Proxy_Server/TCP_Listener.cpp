#include "TCP_Listener.h"

TCPListener::TCPListener(std::string listening_ip_adress, int listening_port, std::string proxy_ip_adress, int proxy_port)
	: m_ip_adress(listening_ip_adress)
	, m_port(listening_port)
	, m_proxy_ip_adress(proxy_ip_adress)
	, m_proxy_port(proxy_port)
	, logger(std::make_unique<Logger>(std::string(LOG_PATH)))
{
}

TCPListener::~TCPListener()
{
	Cleanup();
}

bool TCPListener::Init()
{
	WSADATA wsData;
	WORD word = MAKEWORD(2, 2);

	int wsOK = WSAStartup(word, &wsData);
	if (wsOK != 0)
	{
		log_error("Error initializing WSADATA.");
		return false;
	}

	return true;
}
void TCPListener::Run()
{
	char buf[BUFFER_SIZE];

	m_listening = createListenerSocket();
	if (m_listening == INVALID_SOCKET)
	{
		log_error("Could not create listening socket.");
		return;
	}

	FD_ZERO(&m_sockets);
	FD_SET(m_listening, &m_sockets);

	// Main server loop
	while (true)
	{
		fd_set copy_connections = m_sockets;
		int connections_count = select(0, &copy_connections, nullptr, nullptr, nullptr);
		for (int i = 0; i < connections_count; ++i)
		{
			SOCKET current_socket = copy_connections.fd_array[i];
			if (current_socket == m_listening)
			{
				int acceptOK = acceptNewConnection(buf, BUFFER_SIZE);
				if (acceptOK != 0)
					return;
			}
			else
			{
				char buf[BUFFER_SIZE];
				ZeroMemory(buf, BUFFER_SIZE);

				int byte_received = recv(current_socket, buf, BUFFER_SIZE, 0);

				// If sender is databse
				if (m_connections.find(current_socket) != m_connections.end())
				{
					auto client_iter = m_connections.find(current_socket);

					if (client_iter == m_connections.end())
						continue;

					auto client = client_iter->second;


					// If database sent smth useful
					if (byte_received > 0)
					{
						log("Sending to client\t" + sockaddrToString(client.second) + "\t->\t"
							+ std::to_string(byte_received) + "\tbytes.");
						send(client.first, buf, byte_received, 0);
					}
					// If database closed the connection
					else
					{
						log("Proxy from client\t" + sockaddrToString(client.second) + "\tclosed.");

						// Close corresponding client as well
						closePairOfSockets(current_socket, client.first);
					}
				}
				// If sender is client
				else
				{
					// Find it's server socket
					auto server_iter = std::find_if(m_connections.begin(), m_connections.end(), 
						[current_socket](std::pair<SOCKET, std::pair<SOCKET, sockaddr_in>> const& iter)
						{ return iter.second.first == current_socket; });

					if (server_iter == m_connections.end())
						continue;

					SOCKET server = server_iter->first;
					sockaddr_in from_client_address = server_iter->second.second;

					// If client sent smth useful
					if (byte_received > 0)
					{
						log("Sending to proxy\t" + std::to_string(byte_received) + "\t->\tbytes from\t" + 
							sockaddrToString(from_client_address) + ".");
						send(server, buf, byte_received, 0);

						// If it's not an OK packet for example
						if (byte_received > 4)
						{
							unsigned int size = buf[0];

							// Command code
							unsigned int code = buf[4];

							if ((code == MySQL_Commands::COM_QUERY || code == MySQL_Commands::COM_PREPARE) && size > 5)
							{
								// Log it from the 6th byte, because first 5 bytes contains packet information.
								std::string log_str(buf, byte_received);
								log_str = log_str.substr(5);
								logger->AddLog(log_str);
							}
						}
					}
					// If client closed the connection
					else
					{
						log("Client\t" + sockaddrToString(from_client_address) + "\tclosed.");

						// Close corresponding server as well
						closePairOfSockets(server, current_socket);
					}
				}
			}
		}
	}
}

void TCPListener::Cleanup()
{
	WSACleanup();
}

SOCKET TCPListener::createListenerSocket()
{
	SOCKET new_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (new_socket != INVALID_SOCKET)
	{
		sockaddr_in hint;
		hint.sin_family = AF_INET;
		hint.sin_port = htons(m_port);
		if (m_ip_adress == "")
			hint.sin_addr.S_un.S_addr = INADDR_ANY;
		else
			inet_pton(AF_INET, m_ip_adress.c_str(), &hint.sin_addr);

		int bindOK = bind(new_socket, reinterpret_cast<sockaddr*>(&hint), sizeof(hint));
		if (bindOK != SOCKET_ERROR)
		{
			int listenOK = listen(new_socket, SOMAXCONN);
			if (listenOK == SOCKET_ERROR) 
			{
				return INVALID_SOCKET;
			}
		}
		else
		{
			return INVALID_SOCKET;
		}
	}
	else
	{
		return INVALID_SOCKET;
	}
	return new_socket;
}

int TCPListener::acceptNewConnection(char* buf, int length)
{
	// Create new client socket
	sockaddr_in new_client_adress;
	int size = sizeof(new_client_adress);
	SOCKET new_client = accept(m_listening, reinterpret_cast<sockaddr*>(&new_client_adress), &size);
	FD_SET(new_client, &m_sockets);

	// Create new proxy socket
	SOCKET new_database_connection = socket(AF_INET, SOCK_STREAM, 0);
	if (new_database_connection == INVALID_SOCKET)
	{
		log_error("Error initializing listening socket.");
		return -1;
	}

	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(m_proxy_port);
	inet_pton(AF_INET, m_proxy_ip_adress.c_str(), &hint.sin_addr);

	int connect_result = connect(new_database_connection, reinterpret_cast<sockaddr*>(&hint), sizeof(hint));
	if (connect_result == SOCKET_ERROR)
	{
		log_error("Connection error " + WSAGetLastError());
		return -1;
	}

	FD_SET(new_database_connection, &m_sockets);

	m_connections.insert(std::pair<SOCKET, std::pair<SOCKET, sockaddr_in>>
		(new_database_connection, std::pair<SOCKET, sockaddr_in>(new_client, new_client_adress)));


	log("Established new connection\n" + sockaddrToString(new_client_adress) + 
		"   ->   " + sockaddrToString(hint));


	// Connect these two with exchanging accepting packets
	ZeroMemory(buf, BUFFER_SIZE);
	int bytes = recv(new_database_connection, buf, BUFFER_SIZE, 0);
	send(new_client, buf, bytes, 0);

	return 0;
}

SOCKET TCPListener::waitForConnection(SOCKET listen_socket)
{
	SOCKET client = accept(listen_socket, nullptr, nullptr);
	return client;
}

void TCPListener::closePairOfSockets(SOCKET proxy, SOCKET client)
{
	FD_CLR(client, &m_sockets);
	closesocket(client);

	FD_CLR(proxy, &m_sockets);
	closesocket(proxy);

	m_connections.erase(proxy);
}

std::string TCPListener::sockaddrToString(sockaddr_in& address)
{
	char address_str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(address.sin_addr), address_str, INET_ADDRSTRLEN);

	return std::string(address_str) + ":" + std::to_string(address.sin_port);
}

void TCPListener::log(std::string str)
{
	std::cout << str << "\n\n";
}

void TCPListener::log_error(std::string str)
{
	std::cout << "ERROR: " << str << "\n\n";
}


