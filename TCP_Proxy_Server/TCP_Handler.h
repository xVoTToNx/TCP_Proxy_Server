#pragma once

#include "Utility.h"
#include "Logger.h"

class TCPListener;

class TCPHandler
{
public:
	TCPHandler(TCPListener* tcp_listener, connection_map_ptr connections);
	~TCPHandler();

	void AddDataToHandle(SOCKET socket, std::string&& data);

private:
	void handleThread();
	void handleDataString(data_pair&& data);
	void handleDataFromDatabase(data_pair&& data, SOCKET const client_socket);
	void handleDataFromClient(data_pair&& data, SOCKET const database_socket);

	void parseCommandCode(unsigned int const code, data_pair& data);

	TCPListener* m_tcp_listener;
	connection_map_ptr m_connections;
	std::unique_ptr<Logger> logger;


	std::list<data_pair> data_pool;

	std::condition_variable if_pool_empty;
	std::mutex waiting_for_pool;
	std::mutex writing_to_pool_mutex;

	std::thread handling_thread;

	bool quit;
};

