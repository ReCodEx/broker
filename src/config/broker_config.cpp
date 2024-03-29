#include "broker_config.h"


broker_config::broker_config(const YAML::Node &config)
{
	try {
		if (!config.IsMap()) {
			throw config_error("The configuration is not a YAML map");
		}

		if (config["queue_manager"] && config["queue_manager"].IsScalar()) {
			queue_manager_ = config["queue_manager"].as<std::string>();
		}

		// load client address and port
		if (config["clients"] && config["clients"].IsMap()) {
			if (config["clients"]["address"] && config["clients"]["address"].IsScalar()) {
				client_address_ = config["clients"]["address"].as<std::string>();
			} // no throw... can be omitted
			if (config["clients"]["port"] && config["clients"]["port"].IsScalar()) {
				client_port_ = config["clients"]["port"].as<std::uint16_t>();
			} // no throw... can be omitted
		}

		// load worker address and port
		if (config["workers"] && config["workers"].IsMap()) {
			if (config["workers"]["address"] && config["workers"]["address"].IsScalar()) {
				worker_address_ = config["workers"]["address"].as<std::string>();
			} // no throw... can be omitted
			if (config["workers"]["port"] && config["workers"]["port"].IsScalar()) {
				worker_port_ = config["workers"]["port"].as<std::uint16_t>();
			} // no throw... can be omitted
			if (config["workers"]["max_liveness"] && config["workers"]["max_liveness"].IsScalar()) {
				max_worker_liveness_ = config["workers"]["max_liveness"].as<std::size_t>();
			} // no throw... can be omitted
			if (config["workers"]["max_request_failures"] && config["workers"]["max_request_failures"].IsScalar()) {
				max_request_failures_ = config["workers"]["max_request_failures"].as<std::size_t>();
			} // no throw... can be omitted
			if (config["workers"]["ping_interval"] && config["workers"]["ping_interval"].IsScalar()) {
				worker_ping_interval_ = std::chrono::milliseconds(config["workers"]["ping_interval"].as<std::size_t>());
			} // no throw... can be omitted
		}

		// load monitor address and port
		if (config["monitor"] && config["monitor"].IsMap()) {
			if (config["monitor"]["address"] && config["monitor"]["address"].IsScalar()) {
				monitor_address_ = config["monitor"]["address"].as<std::string>();
			} // no throw... can be omitted
			if (config["monitor"]["port"] && config["monitor"]["port"].IsScalar()) {
				monitor_port_ = config["monitor"]["port"].as<std::uint16_t>();
			} // no throw... can be omitted
		}

		// load frontend address and port
		if (config["notifier"] && config["notifier"].IsMap()) {
			if (config["notifier"]["address"] && config["notifier"]["address"].IsScalar()) {
				notifier_config_.address = config["notifier"]["address"].as<std::string>();
			} // no throw... can be omitted
			if (config["notifier"]["port"] && config["notifier"]["port"].IsScalar()) {
				notifier_config_.port = config["notifier"]["port"].as<std::uint16_t>();
			} // no throw... can be omitted
			if (config["notifier"]["username"] && config["notifier"]["username"].IsScalar()) {
				notifier_config_.username = config["notifier"]["username"].as<std::string>();
			} // no throw... can be omitted
			if (config["notifier"]["password"] && config["notifier"]["password"].IsScalar()) {
				notifier_config_.password = config["notifier"]["password"].as<std::string>();
			} // no throw... can be omitted
		} // no throw... can be omitted

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
				log_config_.log_file_size = config["logger"]["max-size"].as<std::size_t>();
			} // no throw... can be omitted
			if (config["logger"]["rotations"] && config["logger"]["rotations"].IsScalar()) {
				log_config_.log_files_count = config["logger"]["rotations"].as<std::size_t>();
			} // no throw... can be omitted
		} // no throw... can be omitted
	} catch (YAML::Exception &ex) {
		throw config_error("Default broker configuration was not loaded: " + std::string(ex.what()));
	}
}

const std::string &broker_config::get_queue_manager() const
{
	return queue_manager_;
}

const std::string &broker_config::get_client_address() const
{
	return client_address_;
}

std::uint16_t broker_config::get_client_port() const
{
	return client_port_;
}

const std::string &broker_config::get_worker_address() const
{
	return worker_address_;
}

std::uint16_t broker_config::get_worker_port() const
{
	return worker_port_;
}

const std::string &broker_config::get_monitor_address() const
{
	return monitor_address_;
}

std::uint16_t broker_config::get_monitor_port() const
{
	return monitor_port_;
}

std::size_t broker_config::get_max_worker_liveness() const
{
	return max_worker_liveness_;
}

std::chrono::milliseconds broker_config::get_worker_ping_interval() const
{
	return worker_ping_interval_;
}

const log_config &broker_config::get_log_config() const
{
	return log_config_;
}

const notifier_config &broker_config::get_notifier_config() const
{
	return notifier_config_;
}

std::size_t broker_config::get_max_request_failures() const
{
	return max_request_failures_;
}

config_error::config_error(const std::string &msg) : std::runtime_error(msg)
{
}
