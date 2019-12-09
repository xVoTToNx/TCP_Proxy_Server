#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include "TCP_Listener.h"

#pragma comment(lib, "ws2_32.lib")

// DO MAGIC

int main()
{
	TCPListener server("", 54000, "127.0.0.1", 3306);

	if (server.Init())
		server.Run();
	else
		std::cerr << "Could not initialize proxy-server.";
}