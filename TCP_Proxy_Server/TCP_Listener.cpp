#include "TCP_Listener.h"

TCPListener::TCPListener(std::string listening_ip_adress, int listening_port, std::string proxy_ip_adress, int proxy_port)
	: m_ip_adress(listening_ip_adress)
	, m_port(listening_port)
	, m_proxy_ip_adress(proxy_ip_adress)
	, m_proxy_port(proxy_port)
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
		std::cerr << "Error initializing WSADATA.\n";
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
		std::cerr << "Could not create listening socket.\n";
		return;
	}

	FD_ZERO(&m_sockets);

	FD_SET(m_listening, &m_sockets);

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
				{
					std::cin.get();
					return;
				}
			}
			else
			{
				char buf[BUFFER_SIZE];
				ZeroMemory(buf, BUFFER_SIZE);

				int byte_received = recv(current_socket, buf, BUFFER_SIZE, 0);

				if (byte_received > 0)
				{
					if (m_connections.find(current_socket) != m_connections.end())
					{
						auto client = m_connections.find(current_socket)->second;
						char address_str[INET_ADDRSTRLEN];
						inet_ntop(AF_INET, &(client.second.sin_addr), address_str, INET_ADDRSTRLEN);
						std::cout << "Sending to client     " << address_str << "     " << byte_received << "     bytes.\n\n";
						send(client.first, buf, byte_received, 0);
					}
					else
					{
						auto proxy_iter = std::find_if(m_connections.begin(), m_connections.end(), 
							[current_socket](std::pair<SOCKET, std::pair<SOCKET, sockaddr_in>> const& iter)
							{ return iter.second.first == current_socket; });

						SOCKET proxy = proxy_iter->first;
						sockaddr_in from_client_address = proxy_iter->second.second;
						char address_str[INET_ADDRSTRLEN];
						inet_ntop(AF_INET, &(from_client_address.sin_addr), address_str, INET_ADDRSTRLEN);
						std::cout << "Sending to proxy     " << byte_received << "     bytes from     " << address_str << ".\n\n";
						send(proxy, buf, byte_received, 0);
					}
				}
				else
				{
					if (m_connections.find(current_socket) != m_connections.end())
					{
						auto client = m_connections.find(current_socket)->second;
						char address_str[INET_ADDRSTRLEN];
						inet_ntop(AF_INET, &(client.second.sin_addr), address_str, INET_ADDRSTRLEN);
						std::cout << "Proxy from client     " << address_str << "     closed.\n\n";
						FD_CLR(current_socket, &m_sockets);
						closesocket(current_socket);

						//FD_CLR(client.first, &m_sockets);
						//closesocket(client.first);

						m_connections.erase(current_socket);
					}
					else
					{
						auto proxy_iter = std::find_if(m_connections.begin(), m_connections.end(),
							[current_socket](std::pair<SOCKET, std::pair<SOCKET, sockaddr_in>> const& iter)
						{ return iter.second.first == current_socket; });

						SOCKET proxy = proxy_iter->first;
						sockaddr_in from_client_address = proxy_iter->second.second;
						char address_str[INET_ADDRSTRLEN];
						inet_ntop(AF_INET, &(from_client_address.sin_addr), address_str, INET_ADDRSTRLEN);
						std::cout << "Client     " << address_str << "     closed.\n\n";
						
						FD_CLR(current_socket, &m_sockets);
						closesocket(current_socket);

						//FD_CLR(proxy, &m_sockets);
						//closesocket(proxy);

						//m_connections.erase(proxy);
					}
				}

			}
		}
	}
}

void TCPListener::Send(int client_socket, std::string const& message)
{
	send(client_socket, message.c_str(), message.size() + 1, 0);
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
		std::cerr << "Error initializing listening socket.\n";
		return -1;
	}

	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(m_proxy_port);
	inet_pton(AF_INET, m_proxy_ip_adress.c_str(), &hint.sin_addr);

	int connect_result = connect(new_database_connection, reinterpret_cast<sockaddr*>(&hint), sizeof(hint));
	if (connect_result == SOCKET_ERROR)
	{
		std::cerr << "Connection error " << WSAGetLastError() << "\n";
		return -1;
	}

	FD_SET(new_database_connection, &m_sockets);

	m_connections.insert(std::pair<SOCKET, std::pair<SOCKET, sockaddr_in>>
		(new_database_connection, std::pair<SOCKET, sockaddr_in>(new_client, new_client_adress)));


	// Log into console
	char address_str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(new_client_adress.sin_addr), address_str, INET_ADDRSTRLEN);

	char address_str_2[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(hint.sin_addr), address_str_2, INET_ADDRSTRLEN);

	std::cout << "Established new connection\n" << address_str << ":" << new_client_adress.sin_port
		<< "   ->   " << address_str_2 << ":" << hint.sin_port << "\n\n";


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

