#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include "TCP_Listener.h"

#pragma comment(lib, "ws2_32.lib")

int main()
{
	TCPListener server("", 54000, "127.0.0.1", 3306, std::string(LOG_PATH));

	if (server.Init())
		server.Run();
	else
		std::cerr << "Could not initialize proxy-server.";
}