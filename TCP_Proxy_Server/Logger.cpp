#include "Logger.h"



Logger::Logger(std::string log_file_path)
	: log_file(log_file_path, std::fstream::in | std::fstream::app)
	, quit(0)
{
	if (!log_file.is_open())
		throw std::logic_error("Log File Not Found");

	std::thread writing_to_file(&Logger::writeToFile, this, &quit);
	writing_to_file.detach();
}

Logger::~Logger()
{
	log_file.close();
	quit = 1;
	if_pool_empty.notify_one();
}

void Logger::AddLog(std::string& const str)
{
	std::unique_lock<std::mutex> lock(writing_to_pool_mutex);

	string_pool.push_back(str);

	if_pool_empty.notify_one();
}

void Logger::writeToFile(int* quit)
{
	std::unique_lock<std::mutex> lock(waiting_for_pool);
	while (!(*quit))
	{
		if (string_pool.size() == 0)
		{
			if_pool_empty.wait(lock);
		}

		log_file << *string_pool.cbegin() << std::endl;

		std::unique_lock<std::mutex> lock_file(writing_to_pool_mutex);
		string_pool.pop_front();
		lock_file.unlock();
	}
}

