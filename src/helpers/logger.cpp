#include "logger.h"

std::shared_ptr<spdlog::logger> helpers::create_null_logger()
{
	// Create logger manually to avoid global registration of logger
	auto sink = std::make_shared<spdlog::sinks::null_sink_st>();
	auto logger = std::make_shared<spdlog::logger>("", sink);

	return logger;
}

spdlog::level::level_enum helpers::get_log_level(const std::string &lev)
{
	if (lev == "off") {
		return spdlog::level::off;
	} else if (lev == "critical") {
		return spdlog::level::critical;
	} else if (lev == "err") {
		return spdlog::level::err;
	} else if (lev == "warn") {
		return spdlog::level::warn;
	} else if (lev == "info") {
		return spdlog::level::info;
	} else if (lev == "debug") {
		return spdlog::level::debug;
	}

	return spdlog::level::trace;
}

int helpers::get_log_level_number(spdlog::level::level_enum lev)
{
	using namespace spdlog::level;

	switch (lev) {
	case trace: return 6;
	case debug: return 5;
	case info: return 4;
	case warn: return 3;
	case err: return 2;
	case critical: return 1;
	default: return 0;
	}
}

int helpers::compare_log_levels(spdlog::level::level_enum first, spdlog::level::level_enum second)
{
	return get_log_level_number(first) - get_log_level_number(second);
}
