#include "broker_config.h"

broker_config::broker_config()
{
}

broker_config::broker_config(YAML::Node &config)
{
	if (!config.IsMap()) {
		throw config_error("The configuration is not a YAML map");
	}

	if (!config["client_port"].IsScalar()) {
		throw config_error("Client port is not scalar");
	}

	client_port = config["client_port"].as<uint16_t>(0);

	if (!config["worker_port"].IsScalar()) {
		throw config_error("Worker port is not scalar");
	}

	worker_port = config["worker_port"].as<uint16_t>(0);
}

uint16_t broker_config::get_client_port() const
{
	return client_port;
}

uint16_t broker_config::get_worker_port() const
{
	return worker_port;
}
