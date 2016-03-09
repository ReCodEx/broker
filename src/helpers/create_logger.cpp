#include "create_logger.h"

std::shared_ptr<spdlog::logger> helpers::create_null_logger()
{
	// Create logger manually to avoid global registration of logger
	auto sink = std::make_shared<spdlog::sinks::null_sink_st>();
	auto logger = std::make_shared<spdlog::logger>("", sink);

	return logger;
}
