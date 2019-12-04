#include <iostream>
#include <WS2tcpip.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

int main()
{
	std::string ip_adress = "127.0.0.1";
	int port = 54000;

	WSADATA wsData;
	WORD word = MAKEWORD(2, 2);

	int wsOK = WSAStartup(word, &wsData);
	if (wsOK != 0)
	{
		std::cerr << "Error initializing WSADATA.\n";
		return -1;
	}

	SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket == INVALID_SOCKET)
	{
		std::cerr << "Error initializing listening socket.\n";
		WSACleanup();
		return -1;
	}

	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(54000);
	inet_pton(AF_INET, ip_adress.c_str(), &hint.sin_addr);

	int connect_result = connect(client_socket	, reinterpret_cast<sockaddr*>(&hint), sizeof(hint));
	if (connect_result == SOCKET_ERROR)
	{
		std::cerr << "Connection error " << WSAGetLastError() << "\n";
		closesocket(client_socket);
		WSACleanup();
		return -1;
	}

	char buf[4096]; // !!!
	std::string user_input;

	do
	{
		std::cout << "> ";
		std::getline(std::cin, user_input);
		if (user_input != "")
		{
			int send_result = send(client_socket, user_input.c_str(), user_input.size() + 1, 0);
			if (send_result != SOCKET_ERROR)
			{
				ZeroMemory(buf, 4096);
				int bytes_received = recv(client_socket, buf, 4096, 0);
				if (bytes_received > 0)
				{
					std::cout << "SERVER > " << std::string(buf, 0, bytes_received) << "\n";
				}
			}
		}
	} while (user_input.size() > 0);

	closesocket(client_socket);
	WSACleanup();

	return 0;
}