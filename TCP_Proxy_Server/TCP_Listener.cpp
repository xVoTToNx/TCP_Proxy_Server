#include "TCP_Listener.h"

TCPListener::TCPListener(std::string ip_adress, int port, MessageHandler handler)
	: m_ip_adress(ip_adress)
	, m_port(port)
	, m_handler(handler)
{
}

TCPListener::~TCPListener()
{
	Cleanup();
}

bool TCPListener::Init()
{
	WSADATA wsData;
	WORD word = MAKEWORD(2, 2);

	int wsOK = WSAStartup(word, &wsData);
	if (wsOK != 0)
	{
		std::cerr << "Error initializing WSADATA.\n";
		return false;
	}

	return true;
}
void TCPListener::Run()
{
	char buf[BUFFER_SIZE];
	while (true)
	{
		SOCKET db;
		SOCKET client;
		SOCKET listening = CreateListenerSocket();
		if (listening == INVALID_SOCKET)
		{
			break;
		}

		fd_set connections;
		FD_ZERO(&connections);

		FD_SET(listening, &connections);

		while (true)
		{
			fd_set copy_connections = connections;
			int connections_count = select(0, &copy_connections, nullptr, nullptr, nullptr);
			for (int i = 0; i < connections_count; ++i)
			{
				SOCKET current_socket = copy_connections.fd_array[i];
				if (current_socket == listening)
				{
					SOCKET new_client = accept(listening, nullptr, nullptr);
					client = new_client;
					FD_SET(new_client, &connections);









					db = socket(AF_INET, SOCK_STREAM, 0);
					if (db == INVALID_SOCKET)
					{
						std::cerr << "Error initializing listening socket.\n";
						return;
					}

					sockaddr_in hint;
					hint.sin_family = AF_INET;
					hint.sin_port = htons(3306);
					std::string ip_adress("127.0.0.1");
					inet_pton(AF_INET, ip_adress.c_str(), &hint.sin_addr);

					int connect_result = connect(db, reinterpret_cast<sockaddr*>(&hint), sizeof(hint));
					if (connect_result == SOCKET_ERROR)
					{
						std::cerr << "Connection error " << WSAGetLastError() << "\n";
						return;
					}

					FD_SET(db, &connections);

					ZeroMemory(buf, BUFFER_SIZE);
					int bytes = recv(db, buf, BUFFER_SIZE, 0);
					send(new_client, buf, bytes, 0);


















					//for (int i = 0; i < connections.fd_count; ++i)
					//{
					//	SOCKET out_socket = connections.fd_array[i];
					//	if (out_socket != listening)
					//	{
					//		std::ostringstream sstream;
					//		sstream << new_client << "  ENTERED THE SERVER\n\r";
					//		std::string message = sstream.str();
					//		//send(out_socket, message.c_str(), message.size() + 1, 0);
					//	}
					//}
				}
				else
				{
					char buf[BUFFER_SIZE];
					ZeroMemory(buf, BUFFER_SIZE);

					int byte_received = recv(current_socket, buf, BUFFER_SIZE, 0);

					if (byte_received > 0)
					{
						if (current_socket == db)
						{
							std::cout << "TO CLIENT\n";
							send(client, buf, byte_received, 0);
						}
						else if (current_socket == client)
						{
							std::cout << "TO DB\n";
							send(db, buf, byte_received, 0);
						}
						//if (std::string(buf, 0, byte_received) != "\r\n" &&
						//	std::string(buf, 0, byte_received) != "\n")
						//{
						//	for (int i = 0; i < connections.fd_count; ++i)
						//	{
						//		SOCKET out_socket = connections.fd_array[i];
						//		if (out_socket != listening && out_socket == current_socket)
						//		{
						//			std::string to_send(buf, 0, byte_received + 1);

						//			std::ostringstream sstream;
						//			sstream << current_socket << ": " << to_send;
						//			if (to_send[to_send.length() - 1] != 10)
						//				sstream << "\n\r";
						//			std::string message = sstream.str();
						//			std::cout << "START SEND\n";
						//			send(out_socket, message.c_str(), message.size(), 0);
						//			std::cout << "END SEND\n";
						//		}
						//	}
						//}
					}
					else
					{
						FD_CLR(current_socket, &connections);
						closesocket(current_socket);



						for (int i = 0; i < connections.fd_count; ++i)
						{
							SOCKET out_socket = connections.fd_array[i];
							if (out_socket != listening && out_socket != current_socket)
							{
								std::ostringstream sstream;
								sstream << current_socket << "  LEFT THE SERVER\n\r";
								std::string message = sstream.str();
								send(out_socket, message.c_str(), message.size() + 1, 0);
							}
						}
					}

				}
			}
		}
	}
}

void TCPListener::Send(int client_socket, std::string const& message)
{
	send(client_socket, message.c_str(), message.size() + 1, 0);
}

void TCPListener::Cleanup()
{
	WSACleanup();
}

SOCKET TCPListener::CreateListenerSocket()
{
	SOCKET new_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (new_socket != INVALID_SOCKET)
	{
		sockaddr_in hint;
		hint.sin_family = AF_INET;
		hint.sin_port = htons(m_port);
		if (m_ip_adress == "")
			hint.sin_addr.S_un.S_addr = INADDR_ANY;
		else
			inet_pton(AF_INET, m_ip_adress.c_str(), &hint.sin_addr);

		int bindOK = bind(new_socket, reinterpret_cast<sockaddr*>(&hint), sizeof(hint));
		if (bindOK != SOCKET_ERROR)
		{
			int listenOK = listen(new_socket, SOMAXCONN);
			if (listenOK == SOCKET_ERROR) 
			{
				return INVALID_SOCKET;
			}
		}
		else
		{
			return INVALID_SOCKET;
		}
	}
	else
	{
		return INVALID_SOCKET;
	}
	return new_socket;
}

SOCKET TCPListener::WaitForConnection(SOCKET listen_socket)
{
	SOCKET client = accept(listen_socket, nullptr, nullptr);
	return client;
}

