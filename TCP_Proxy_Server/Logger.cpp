#include "Logger.h"



Logger::Logger(std::string const log_file_path)
	: log_file(log_file_path, std::fstream::in | std::fstream::app)
{
	if (!log_file.is_open())
		throw std::logic_error("Log File Not Found");
}

Logger::~Logger()
{
	log_file.close();
}

void Logger::AddLog(std::string const& str)
{
	log_file << str << std::endl;
}

