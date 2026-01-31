#include "utils/TLog.hpp"

#include <memory>
#include <string>
#include <vector>

#include "spdlog/async.h"
#include "spdlog/common.h"
#include "spdlog/logger.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace spd = spdlog;

using namespace std;

namespace gentau {
static vector<spd::sink_ptr> createSinks()
{
	static vector<spd::sink_ptr> sinks = []() {
		vector<spd::sink_ptr> s_list;
		auto                  stdout_sink = make_shared<spd::sinks::stdout_color_sink_mt>();

		spd::init_thread_pool(8192, 1);

		if constexpr (conf::TLogToFile) {
			auto file_sink = make_shared<spd::sinks::rotating_file_sink_mt>(
				"logs/gt_framework.log",
				1024 * 1024 * 10,  // 10 MB
				5                  // 5 files
			);

			file_sink->set_level(spd::level::trace);
			s_list.push_back(file_sink);
		}

		stdout_sink->set_level(spd::level::debug);
		s_list.push_back(stdout_sink);

		return s_list;
	}();  // Return value of lambda immediately.

	return sinks;
}

static LoggerPtr createAsyncLogger(const string& logger_name)
{
	auto sinks = createSinks();

	auto logger = make_shared<spd::async_logger>(
		logger_name,
		begin(sinks),
		end(sinks),
		spd::thread_pool(),
		spd::async_overflow_policy::overrun_oldest
	);

	logger->set_level(static_cast<spd::level::level_enum>(SPDLOG_ACTIVE_LEVEL));

	logger->flush_on(spd::level::err);

	spd::register_logger(logger);
	return logger;
}

LoggerPtr getImgTransLogger()
{
	static LoggerPtr logger = createAsyncLogger("ImgTrans");
	return logger;
}

LoggerPtr getProtoLogger()
{
	static LoggerPtr logger = createAsyncLogger("Proto");
	return logger;
}

}  // namespace gentau