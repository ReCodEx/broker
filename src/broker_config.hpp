#ifndef CODEX_BROKER_BROKER_CONFIG_HPP
#define CODEX_BROKER_BROKER_CONFIG_HPP


#include <iostream>

class broker_config {
public:
	/**
	 * A constructor that loads the configuration from a file
	 * @param in The input stream
	 */
	broker_config (std::istream &in);

	/**
	 * Get the port to listen for incoming tasks
	 */
	uint16_t get_client_port () const;

	/**
	 * Get the port for communication with workers
	 */
	uint16_t get_worker_port () const;
};


#endif //CODEX_BROKER_BROKER_CONFIG_HPP
