#include "utils/TLog.hpp"

#include <string>
#include <iostream>
#include <chrono>
#include <thread>

#define T_LOG_TAG_IMG "[LOG-TEST] "
#define T_LOG_TAG_PROTO "[LOG-TEST] "

std::string heavy_func() 
{
	unsigned long long sum = 0;
	for(int i = 0; i < 10; i++) {
		sum += i * i;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return std::to_string(sum);
}

int main()
{
	tImgTransLogTrace("This is a TRACE log message.");
	tImgTransLogDebug("This is a DEBUG log message.");
	tImgTransLogInfo("This is an INFO log message.");
	tImgTransLogWarn("This is a WARN log message.");
	tImgTransLogError("This is an ERROR log message.");
	tImgTransLogCritical("This is a CRITICAL log message.");

    tProtoLogInfo("Doing very heavy func {}", heavy_func());

	std::chrono::system_clock::time_point now_t = std::chrono::system_clock::now();
	auto now_t_lit = std::chrono::system_clock::to_time_t(now_t);

	std::cout << "We should see this very soon:" << std::ctime(&now_t_lit) << std::endl;

    spdlog::shutdown();

	return 0;
}