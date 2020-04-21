#pragma once

#include <string>
#include <algorithm>
#include <iostream>
#include <WS2tcpip.h>
#include <map>
#include <memory>
#include <assert.h>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE (16777216) // > 16 MB
#define LOG_PATH ("log.txt")
#define COMMAND_BYTE_INDEX 4

// std::map<DB_SOCKET, CLIENT_SOCKET>
typedef std::shared_ptr<std::map<SOCKET, SOCKET>> connection_map_ptr;
typedef std::pair<SOCKET, std::string> data_pair;

enum MySQL_Commands
{
	// Tell mysql server to execute a query
	COM_QUERY = 0x3,

	// Tel mysql server to prepare a statement for execution
	COM_PREPARE = 0x16
};

void static log(std::string&& str)
{
	std::cout << str << "\n\n";
}

void static log_error(std::string&& str)
{
	std::cout << "ERROR: " << str << "\n\n";
}


