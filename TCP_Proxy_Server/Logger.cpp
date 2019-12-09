#include "Logger.h"



Logger::Logger(std::string log_file_path)
	:log_file(log_file_path, std::fstream::in | std::fstream::app)
{
	if (!log_file.is_open())
		throw std::logic_error("Log File Not Found");
}

Logger::~Logger()
{
	log_file.close();
}

void Logger::AddLog(std::string& const str)
{
	std::unique_lock<std::mutex> lock(writing_to_pool_mutex);

		string_pool.push_back(str);

	lock.unlock();

	std::async(std::launch::async, &Logger::writeToFile, this);
}

void Logger::writeToFile()
{
	std::unique_lock<std::mutex> lock(writing_to_pool_mutex);

	std::string log(std::move(*string_pool.begin()));
	string_pool.pop_front();

	lock.unlock();



	std::unique_lock<std::mutex> lock_file(writing_to_file_mutex);

		log_file << log << std::endl;

	lock_file.unlock();
}

