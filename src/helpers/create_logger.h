#ifndef CODEX_BROKER_CREATE_LOGGER_H
#define CODEX_BROKER_CREATE_LOGGER_H

#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/null_sink.h>

namespace helpers
{
	std::shared_ptr<spdlog::logger> create_null_logger();
}


#endif // CODEX_BROKER_CREATE_LOGGER_H
