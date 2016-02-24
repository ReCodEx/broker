#include "broker_config.h"

broker_config::broker_config()
{
}

broker_config::broker_config(const YAML::Node &config)
{
	try {
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

		// load logger
		if (config["logger"] && config["logger"].IsMap()) {
			if (config["logger"]["file"] && config["logger"]["file"].IsScalar()) {
				fs::path tmp = config["logger"]["file"].as<std::string>();
				log_config_.log_basename = tmp.filename().string();
				log_config_.log_path = tmp.parent_path().string();
			} // no throw... can be omitted
			if (config["logger"]["level"] && config["logger"]["level"].IsScalar()) {
				log_config_.log_level = config["logger"]["level"].as<std::string>();
			} // no throw... can be omitted
			if (config["logger"]["max-size"] && config["logger"]["max-size"].IsScalar()) {
				log_config_.log_file_size = config["logger"]["max-size"].as<size_t>();
			} // no throw... can be omitted
			if (config["logger"]["rotations"] && config["logger"]["rotations"].IsScalar()) {
				log_config_.log_files_count = config["logger"]["rotations"].as<size_t>();
			} // no throw... can be omitted
		} // no throw... can be omitted
	} catch (YAML::Exception &ex) {
		throw config_error("Default broker configuration was not loaded: " + std::string(ex.what()));
	}
}

uint16_t broker_config::get_client_port() const
{
	return client_port;
}

uint16_t broker_config::get_worker_port() const
{
	return worker_port;
}

const log_config &broker_config::get_log_config() const
{
	return log_config_;
}
