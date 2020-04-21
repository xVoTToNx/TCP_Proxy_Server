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
public:
	Logger(std::string const log_file_path);
	~Logger();

	void AddLog(std::string const& str);

private:
	std::ofstream log_file;
};
