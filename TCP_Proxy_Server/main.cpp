#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include "TCP_Listener.h"

#pragma comment(lib, "ws2_32.lib")

void HandleMyMessages(TCPListener* listener, int client_socket, std::string message);

int main()
{
	TCPListener server("", 54000, "127.0.0.1", 3306);

	if (server.Init())
		server.Run();
}

void HandleMyMessages(TCPListener* listener, int client_socket, std::string message)
{
	listener->Send(client_socket, message);
}