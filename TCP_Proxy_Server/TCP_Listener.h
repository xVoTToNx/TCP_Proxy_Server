#pragma once

#include <string>
#include <algorithm>
#include <iostream>
#include <WS2tcpip.h>
#include <map>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE (4096)

class TCPListener
{
public:
	TCPListener(std::string listening_ip_adress, int listening_port, std::string proxy_ip_adress, int proxy_port);
	~TCPListener();

	bool Init();
	void Run();
	void Send(int client_socket, std::string const& message);
	void Cleanup();

private:
	SOCKET createListenerSocket();
	int acceptNewConnection(char* buf, int length);
	SOCKET waitForConnection(SOCKET listen_socket);

	std::string m_ip_adress;
	int m_port;
	std::string m_proxy_ip_adress;
	int m_proxy_port;

	fd_set m_sockets;
	std::map<SOCKET, std::pair<SOCKET, sockaddr_in>> m_connections;
	SOCKET m_listening;
};

