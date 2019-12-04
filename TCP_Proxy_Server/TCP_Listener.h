#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE (4096)

class TCPListener;
typedef void (*MessageHandler)(TCPListener* listner, int client_socket, std::string message);

class TCPListener
{
public:
	TCPListener(std::string ip_adress, int port, MessageHandler handler);
	~TCPListener();

	bool Init();
	void Run();
	void Send(int client_socket, std::string const& message);
	void Cleanup();

private:
	SOCKET CreateListenerSocket();
	SOCKET WaitForConnection(SOCKET listen_socket);

	std::string m_ip_adress;
	int m_port;
	MessageHandler m_handler;
};

