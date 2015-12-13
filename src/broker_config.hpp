#ifndef CODEX_BROKER_BROKER_CONFIG_HPP
#define CODEX_BROKER_BROKER_CONFIG_HPP


#include <iostream>
#include <yaml-cpp/yaml.h>

class broker_config {
private:
	uint16_t client_port = 0;
	uint16_t worker_port = 0;

public:
	/** A default constructor */
	broker_config ();

	/**
	 * A constructor that loads the configuration from a YAML document
	 * @param config The input document
	 */
	broker_config (YAML::Node &config);

	/**
	 * Get the port to listen for incoming tasks
	 */
	virtual uint16_t get_client_port () const;

	/**
	 * Get the port for communication with workers
	 */
	virtual uint16_t get_worker_port () const;
};

class config_error : std::runtime_error {
public:
	explicit config_error (const std::string &msg) : std::runtime_error(msg)
	{
	}

};

#endif //CODEX_BROKER_BROKER_CONFIG_HPP
