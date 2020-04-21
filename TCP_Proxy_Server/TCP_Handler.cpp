#include "TCP_Handler.h"
#include "TCP_Listener.h"

TCPHandler::TCPHandler(TCPListener* tcp_listener, connection_map_ptr connections)
	: m_tcp_listener(tcp_listener)
	, m_connections(connections)
	, quit(false)
	, handling_thread(&TCPHandler::handleThread, this)
	, logger(std::make_unique<Logger>(std::string(LOG_PATH)))
{
}

TCPHandler::~TCPHandler()
{
	if_pool_empty.notify_one();
	quit = true;
	handling_thread.join();
}

void TCPHandler::AddDataToHandle(SOCKET socket, std::string&& data)
{
	std::unique_lock<std::mutex> lock(writing_to_pool_mutex);

	data_pool.emplace_back(data_pair(socket, data));

	if_pool_empty.notify_one();
}

void TCPHandler::handleThread()
{

	std::unique_lock<std::mutex> lock(waiting_for_pool);
	while (!quit)
	{

		// If the thread spontaneously wakes up, but no data ready
		while (data_pool.size() == 0)
		{
			if_pool_empty.wait(lock);

			if (!quit) break;
		}

		// Grab one string from pool
		std::unique_lock<std::mutex> lock_data_pool(writing_to_pool_mutex);

		data_pair data(std::move(*data_pool.begin()));
		data_pool.pop_front();

		lock_data_pool.unlock();

		handleDataString(std::move(data));
	}
}

void TCPHandler::handleDataString(data_pair&& data)
{
	SOCKET current_socket = data.first;

	auto connection_iterator = m_connections->find(current_socket);

	// if data from database
	if (connection_iterator != m_connections->end())
	{
		SOCKET client_socket = connection_iterator->second;
		handleDataFromDatabase(std::move(data), client_socket);
		return;
	}

	connection_iterator = std::find_if(m_connections->begin(), m_connections->end(),
		[current_socket](std::pair<SOCKET, SOCKET> const& iter)
	{ return iter.second == current_socket; });

	// If data from client
	if (connection_iterator != m_connections->end())
	{
		SOCKET database_socket = connection_iterator->first;
		handleDataFromClient(std::move(data), database_socket);
		return;
	}
}

void TCPHandler::handleDataFromDatabase(data_pair&& data, SOCKET const client_socket)
{
	unsigned int data_size = data.second.size();
	SOCKET database_socket = data.first;

	// If database closed the connection
	
	if (data_size == 0)
	{
		log("Databse from client\t" + std::to_string(client_socket) + "\tclosed.");

		// Close corresponding client as well
		m_tcp_listener->closePairOfSockets(database_socket, client_socket);
		return;
	}

	// Else database sent smth useful
	log("Sending to client\t" + std::to_string(client_socket) + "\t->\t"
		+ std::to_string(data_size) + "\tbytes.");

	m_tcp_listener->sendToClient(std::move(data), client_socket);
}

void TCPHandler::handleDataFromClient(data_pair&& data, SOCKET const database_socket)
{
	unsigned int data_size = data.second.size();
	SOCKET client_socket = data.first;
	std::string &data_str = data.second;

	// If client closed the connection
	if (data_size == 0)
	{
		log("Client\t" + std::to_string(client_socket) + "\tclosed.");

		// Close corresponding server as well
		m_tcp_listener->closePairOfSockets(database_socket, client_socket);
	}

	// Command code
	unsigned int code = data_str[COMMAND_BYTE_INDEX];
	parseCommandCode(code, data);

	// Else client sent smth useful
	log("Sending to database\t" + std::to_string(data_size) + "\t->\tbytes from\t" +
		std::to_string(client_socket) + ".");

	m_tcp_listener->sendToDatabase(std::move(data), database_socket);
}


void TCPHandler::parseCommandCode(unsigned int const code, data_pair& data)
{
	std::string &data_str = data.second;

	switch (code)
	{
	case MySQL_Commands::COM_QUERY:
	case MySQL_Commands::COM_PREPARE:
	{
		// Log the data from the 6th byte, because first 5 bytes contain packet information.

		std::string log_str = std::move(data_str.substr(5));
		logger->AddLog(log_str);
	}
	}
}




