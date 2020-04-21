#include "TCP_Listener.h"

TCPListener::TCPListener(std::string listening_ip_adress, int listening_port, std::string proxy_ip_adress, int proxy_port, std::string log_path)
	: m_ip_adress(listening_ip_adress)
	, m_port(listening_port)
	, m_proxy_ip_adress(proxy_ip_adress)
	, m_proxy_port(proxy_port)
	, m_connections(std::make_shared<std::map<SOCKET, SOCKET>>())
	, tcp_handler(std::make_unique<TCPHandler>(this, m_connections))
{
}

TCPListener::~TCPListener()
{
	WSACleanup();
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

	m_listening_socket = createListenerSocket();
	if (m_listening_socket == INVALID_SOCKET)
	{
		log_error("Could not create listening socket.");
		return false;
	}

	FD_ZERO(&m_sockets);
	FD_SET(m_listening_socket, &m_sockets);

	return true;
}

void TCPListener::Run()
{
	char* data_buffer = new char[BUFFER_SIZE];

	// Main server loop
	while (true)
	{
		fd_set copy_connections = m_sockets;
		int connections_count = select(0, &copy_connections, nullptr, nullptr, nullptr);

		for (int i = 0; i < connections_count; ++i)
		{
			SOCKET current_socket = copy_connections.fd_array[i];
			if (current_socket == m_listening_socket)
			{
				int acceptOK = acceptNewConnection(data_buffer, BUFFER_SIZE);
				if (acceptOK != 0)
					return;
			}

			int bytes_received = recv(current_socket, data_buffer, BUFFER_SIZE, 0);
			bytes_received = bytes_received > 0 ? bytes_received : 0;

			std::string data_str = std::string(data_buffer, bytes_received);
			ZeroMemory(data_buffer, bytes_received);

			tcp_handler->AddDataToHandle(current_socket, std::move(data_str));
		}
	}

	delete data_buffer;
}

SOCKET TCPListener::createListenerSocket()
{
	SOCKET new_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (new_socket == INVALID_SOCKET)
		return INVALID_SOCKET;


	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(m_port);
	if (m_ip_adress == "")
		hint.sin_addr.S_un.S_addr = INADDR_ANY;
	else
		inet_pton(AF_INET, m_ip_adress.c_str(), &hint.sin_addr);

	int bindOK = bind(new_socket, reinterpret_cast<sockaddr*>(&hint), sizeof(hint));
	if (bindOK == SOCKET_ERROR)
		return INVALID_SOCKET;
	
	int listenOK = listen(new_socket, SOMAXCONN);
	if (listenOK == SOCKET_ERROR) 
		return INVALID_SOCKET;

	return new_socket;
}

int TCPListener::acceptNewConnection(char* buf, int length)
{
	// Create new client socket
	sockaddr_in new_client_adress;
	int size = sizeof(new_client_adress);
	SOCKET new_client_socket = accept(m_listening_socket, reinterpret_cast<sockaddr*>(&new_client_adress), &size);
	FD_SET(new_client_socket, &m_sockets);

	// Create new proxy socket
	SOCKET new_database_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (new_database_socket == INVALID_SOCKET)
	{
		log_error("Error initializing listening socket.");
		return -1;
	}

	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(m_proxy_port);
	inet_pton(AF_INET, m_proxy_ip_adress.c_str(), &hint.sin_addr);

	int connect_result = connect(new_database_socket, reinterpret_cast<sockaddr*>(&hint), sizeof(hint));
	if (connect_result == SOCKET_ERROR)
	{
		log_error("Connection error " + WSAGetLastError());
		return -1;
	}

	FD_SET(new_database_socket, &m_sockets);

	m_connections->insert(std::pair<SOCKET, SOCKET>(new_database_socket, new_client_socket));


	log("Established new connection\n" + std::to_string(new_client_socket) +
		"   ->   " + std::to_string(new_database_socket));


	// Connect these two with exchanging accepting packets
	ZeroMemory(buf, length);
	int bytes = recv(new_database_socket, buf, length, 0);
	send(new_client_socket, buf, bytes, 0);
	ZeroMemory(buf, bytes);

	return 0;
}

SOCKET TCPListener::waitForConnection(SOCKET const listen_socket)
{
	SOCKET client = accept(listen_socket, nullptr, nullptr);
	return client;
}

void TCPListener::sendToClient(data_pair&& data, SOCKET const client_socket)
{
	int data_size = data.second.size();
	std::string &data_str = data.second;

	send(client_socket, data_str.c_str(), data_size, 0);
}

void TCPListener::sendToDatabase(data_pair&& data, SOCKET const database_socket)
{
	int data_size = data.second.size();
	std::string &data_str = data.second;

	send(database_socket, data_str.c_str(), data_size, 0);
}

void TCPListener::closePairOfSockets(SOCKET const database, SOCKET const client)
{
	FD_CLR(client, &m_sockets);
	closesocket(client);

	FD_CLR(database, &m_sockets);
	closesocket(database);

	m_connections->erase(database);
}



