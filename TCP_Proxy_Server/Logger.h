#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <list>
#include <future>


class Logger
{
	std::list<std::string> string_pool;
	std::ofstream log_file;
	std::condition_variable if_pool_empty;
	std::mutex writing_to_file_mutex;
	std::mutex writing_to_pool_mutex;

public:
	Logger(std::string log_file_path);
	~Logger();

	void AddLog(std::string& const str);

private:
	void writeToFile();
};
