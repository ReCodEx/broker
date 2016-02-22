#ifndef CODEX_BROKER_LOG_CONFIG_H
#define CODEX_BROKER_LOG_CONFIG_H

#include "spdlog/spdlog.h"


/**
 * Structure which stores all information needed to initialize logger.
 */
struct log_config {
public:
	/**
	 * Determines in which directory log will be saved.
	 */
	std::string log_path = "/var/log/recodex/";
	/**
	 * Filemane of log file without extension.
	 */
	std::string log_basename = "broker";
	/**
	 * Log filename extension.
	 */
	std::string log_suffix = "log";
	/**
	 * Level of logging.
	 * Currently 4 levels are available: emerg, warn, info, debug
	 */
	std::string log_level = "debug";
	/**
	 * File size of one log file.
	 */
	int log_file_size = 1024 * 1024;
	/**
	 * Number of rotations which will be used.
	 */
	int log_files_count = 3;

	/**
	 * For appropriate textual description of logging level returns spdlog::level enumeration.
	 * @param lev textual description of logging level
	 * @return enumeration item suitable to textual description
	 */
	static spdlog::level::level_enum get_level(const std::string &lev)
	{
		if (lev == "off") {
			return spdlog::level::off;
		} else if (lev == "emerg") {
			return spdlog::level::emerg;
		} else if (lev == "alert") {
			return spdlog::level::alert;
		} else if (lev == "critical") {
			return spdlog::level::critical;
		} else if (lev == "err") {
			return spdlog::level::err;
		} else if (lev == "warn") {
			return spdlog::level::warn;
		} else if (lev == "notice") {
			return spdlog::level::notice;
		} else if (lev == "info") {
			return spdlog::level::info;
		} else if (lev == "debug") {
			return spdlog::level::debug;
		}

		return spdlog::level::trace;
	}


	/**
	 * Classical equality operator on log_config structures.
	 * @param second
	 * @return true if this instance and second has the same variables values
	 */
	bool operator==(const log_config &second) const
	{
		return (log_path == second.log_path && log_basename == second.log_basename && log_suffix == second.log_suffix &&
			log_level == second.log_level && log_file_size == second.log_file_size &&
			log_files_count == second.log_files_count);
	}

	/**
	 * Opposite of equality operator
	 * @param second
	 * @return true if compared structured has different values
	 */
	bool operator!=(const log_config &second) const
	{
		return !((*this) == second);
	}
};

#endif // CODEX_BROKER_LOG_CONFIG_H

