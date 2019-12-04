#include <iostream>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

int main()
{
	WSADATA wsData;
	WORD word = MAKEWORD(2, 2);

	int wsOK = WSAStartup(word, &wsData);
	if (wsOK != 0)
	{
		std::cerr << "Error initializing WSADATA.\n";
		return -1;
	}

	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if(listening == INVALID_SOCKET)
	{
		std::cerr << "Error initializing listening socket.\n";
		WSACleanup();
		return -1;
	}

	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(54000);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	bind(listening, reinterpret_cast<sockaddr*>(&hint), sizeof(hint));
	listen(listening, SOMAXCONN);

	sockaddr_in client;
	int client_size = sizeof(client);
	SOCKET client_socket = accept(listening, reinterpret_cast<sockaddr*>(&client), &client_size);


	char host[NI_MAXHOST];
	char service[NI_MAXSERV];

	ZeroMemory(host, NI_MAXHOST);
	ZeroMemory(service, NI_MAXSERV);

	if (getnameinfo(reinterpret_cast<sockaddr*>(&client), client_size, host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
	{
		std::cout << host << " connected (hostname) on port " << service << "\n";
	}
	else
	{
		inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
		std::cout << host << " connected (ip-adress) on port " << ntohs(client.sin_port) << "\n";
	}


	closesocket(listening);


	char buf[4096]; //!!!!!!!!1
	while (true)
	{
		ZeroMemory(buf, 4096);
		int bytes_received = recv(client_socket, buf, 4096, 0);

		if (bytes_received == 0)
		{
			std::cout << "Client disconnected.\n";
			break;
		}


		if (bytes_received == SOCKET_ERROR)
		{
			std::cerr << "Error in receiving bytes from client.\n";
			break;
		}



		send(client_socket, buf, bytes_received + 1, 0);
	}

	closesocket(client_socket);

	WSACleanup();
}