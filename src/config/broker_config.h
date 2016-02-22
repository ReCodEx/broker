#ifndef CODEX_BROKER_BROKER_CONFIG_HPP
#define CODEX_BROKER_BROKER_CONFIG_HPP

#include <iostream>
#include <string>
#include <map>
#include <yaml-cpp/yaml.h>

#define BOOST_FILESYSTEM_NO_DEPRECATED
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include "log_config.h"


/**
 * An object representation of a default broker's configuration
 */
class broker_config
{
public:
	/** A default constructor */
	broker_config();

	/**
	 * A constructor that loads the configuration from a YAML document
	 * @param config The input document
	 */
	broker_config(const YAML::Node &config);

	/**
	 * Get the port to listen for incoming tasks
	 */
	virtual uint16_t get_client_port() const;

	/**
	 * Get the port for communication with workers
	 */
	virtual uint16_t get_worker_port() const;
	/**
	 * Get wrapper for logger configuration.
	 * @return constant reference to log_config structure
	 */
	const log_config &get_log_config();
private:
	/** Client socket port (from frontend) */
	uint16_t client_port = 0;
	/** Server socket port (to workers) */
	uint16_t worker_port = 0;
	/** Configuration of logger */
	log_config log_config_;

};


/**
 * Default broker configuration exception.
 */
class config_error : public std::runtime_error
{
public:
	/**
	 * Construction with message returned with @ref what() method.
	 * @param msg description of exception circumstances
	 */
	explicit config_error(const std::string &msg) : std::runtime_error(msg)
	{
	}
};

#endif // CODEX_BROKER_BROKER_CONFIG_HPP
