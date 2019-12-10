#pragma once

#include <string>
#include <algorithm>
#include <iostream>
#include <WS2tcpip.h>
#include <map>
#include <memory>

#pragma comment(lib, "ws2_32.lib")

#include "Logger.h"

#define BUFFER_SIZE (4096 * 16)
#define LOG_PATH ("log.txt")

class TCPListener
{
public:
	TCPListener(std::string listening_ip_adress, int listening_port, std::string proxy_ip_adress, int proxy_port);
	~TCPListener();

	bool Init();
	void Run();
	void Cleanup();

	enum MySQL_Commands
	{
		// Tell mysql server to execute a query
		COM_QUERY = 3,

		// Tel mysql server to prepare a statement for execution
		COM_PREPARE = 22
	};

private:
	SOCKET createListenerSocket();
	int acceptNewConnection(char* buf, int length);
	SOCKET waitForConnection(SOCKET listen_socket);
	void closePairOfSockets(SOCKET proxy, SOCKET client);
	std::string sockaddrToString(sockaddr_in& address);
	void log(std::string str);
	void log_error(std::string str);



	std::string m_ip_adress;
	int m_port;
	std::string m_proxy_ip_adress;
	int m_proxy_port;

	fd_set m_sockets;
	std::map<SOCKET, std::pair<SOCKET, sockaddr_in>> m_connections;
	SOCKET m_listening;

	std::unique_ptr<Logger> logger;
};

