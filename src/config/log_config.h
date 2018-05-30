#ifndef RECODEX_BROKER_LOG_CONFIG_H
#define RECODEX_BROKER_LOG_CONFIG_H

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
	 * One of: trace, debug, info, notice, warn, err, critical, alert, emerg
	 */
	std::string log_level = "debug";
	/**
	 * File size of one rotation of log file.
	 */
	std::size_t log_file_size = 1024 * 1024;
	/**
	 * Number of rotations which will be kept saved.
	 */
	std::size_t log_files_count = 3;

	/**
	 * Equality operator on @ref log_config structures.
	 * @param second Other config for comparison.
	 * @return @a true if this instance and second has the same variable values, @a false otherwise.
	 */
	bool operator==(const log_config &second) const
	{
		return (log_path == second.log_path && log_basename == second.log_basename && log_suffix == second.log_suffix &&
			log_level == second.log_level && log_file_size == second.log_file_size &&
			log_files_count == second.log_files_count);
	}

	/**
	 * Opposite of equality operator
	 * @param second Other config for comparison.
	 * @return @a true if this instance and second has different variable values, @a false otherwise.
	 */
	bool operator!=(const log_config &second) const
	{
		return !((*this) == second);
	}
};

#endif // RECODEX_BROKER_LOG_CONFIG_H
