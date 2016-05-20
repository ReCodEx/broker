#ifndef CODEX_BROKER_CONFIG_H
#define CODEX_BROKER_CONFIG_H

#include <iostream>
#include <map>
#include <string>
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
	 * Return IP address for client connections (from frontend)
	 */
	virtual const std::string &get_client_address() const;
	/**
	 * Get the port to listen for incoming tasks
	 */
	virtual uint16_t get_client_port() const;
	/**
	 * Return IP address for worker connections
	 */
	virtual const std::string &get_worker_address() const;
	/**
	 * Get the port for communication with workers
	 */
	virtual uint16_t get_worker_port() const;
	/**
	 * Return IP address for monitor connections
	 */
	virtual const std::string &get_monitor_address() const;
	/**
	 * Get the port for communication with monitor
	 */
	virtual uint16_t get_monitor_port() const;
	/**
	 * Get the maximum (i.e. initial) liveness of a worker
	 */
	virtual size_t get_max_worker_liveness() const;
	/**
	 * Get the time (in milliseconds) expected to pass between pings from the worker
	 */
	virtual std::chrono::milliseconds get_worker_ping_interval() const;
	/**
	 * Get wrapper for logger configuration.
	 * @return constant reference to log_config structure
	 */
	const log_config &get_log_config() const;

private:
	/** Client socket address (from frontend) */
	std::string client_address_ = "*"; // '*' is any address
	/** Client socket port (from frontend) */
	uint16_t client_port_ = 0;
	/** Server socket address (to workers) */
	std::string worker_address_ = "*"; // '*' is any address
	/** Server socket port (to workers) */
	uint16_t worker_port_ = 0;
	/** Monitor socket address */
	std::string monitor_address_ = "127.0.0.1";
	/** Monitor socket port */
	uint16_t monitor_port_ = 7894;
	/**
	 * Maximum (initial) liveness of a worker
	 * (the amount of pings the worker can miss before it's considered dead)
	 */
	size_t max_worker_liveness_ = 4;
	/** Time (in milliseconds) expected to pass between pings from the worker */
	std::chrono::milliseconds worker_ping_interval_ = std::chrono::milliseconds(1000);
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

#endif // CODEX_BROKER_CONFIG_H
