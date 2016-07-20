#ifndef RECODEX_BROKER_HELPERS_LOGGER_H
#define RECODEX_BROKER_HELPERS_LOGGER_H

#include <memory>

// clang-format off
#include <spdlog/spdlog.h>
#include <spdlog/sinks/null_sink.h>
// clang-format on

namespace helpers
{
	/**
	 * Creates null logger which can be used as a sink.
	 * @return smart pointer to created logger
	 */
	std::shared_ptr<spdlog::logger> create_null_logger();

	/**
	 * For appropriate textual description of logging level returns spdlog::level enumeration.
	 * @param lev textual description of logging level
	 * @return enumeration item suitable to textual description
	 */
	spdlog::level::level_enum get_log_level(const std::string &lev);

	/**
	 * Get unique identification of given log level.
	 * More informative levels (debug, info) has greater values than error levels.
	 * @param lev spdlog level enum type
	 * @return unique identificator of logging level
	 */
	int get_log_level_number(spdlog::level::level_enum lev);

	/**
	 * Compare given enums and return their difference which is derived from following:
	 *   result = minuend - subtrahend
	 * Where integral values are taken from @a get_log_level_number() function.
	 * @param first minuend
	 * @param second subtrahend
	 * @return difference
	 */
	int compare_log_levels(spdlog::level::level_enum first, spdlog::level::level_enum second);
}


#endif // RECODEX_BROKER_HELPERS_LOGGER_H
