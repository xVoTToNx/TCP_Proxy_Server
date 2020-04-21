#pragma once

#include "Utility.h"
#include "TCP_Handler.h"

class TCPListener
{
public:
	TCPListener(std::string listening_ip_adress, int listening_port, 
		std::string proxy_ip_adress, int proxy_port, std::string log_path);
	~TCPListener();

	bool Init();
	void Run();

private:
	SOCKET createListenerSocket();
	int acceptNewConnection(char* buf, int length);
	SOCKET waitForConnection(SOCKET const listen_socket);

	void sendToClient(data_pair&& data, SOCKET const client_socket);
	void sendToDatabase(data_pair&& data, SOCKET const database_socket);
	void closePairOfSockets(SOCKET const proxy, SOCKET const client);

	std::string m_ip_adress;
	int m_port;
	std::string m_proxy_ip_adress;
	int m_proxy_port;

	fd_set m_sockets;
	connection_map_ptr m_connections;
	SOCKET m_listening_socket;

	std::unique_ptr<TCPHandler> tcp_handler;

	friend TCPHandler;
};

