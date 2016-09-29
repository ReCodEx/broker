#include "broker_connect.h"

const std::string broker_connect::KEY_WORKERS = "workers";
const std::string broker_connect::KEY_CLIENTS = "clients";
const std::string broker_connect::KEY_MONITOR = "monitor";
const std::string broker_connect::KEY_STATUS_NOTIFIER = "status_notifier";
const std::string broker_connect::KEY_TIMER = reactor::KEY_TIMER;

broker_connect::broker_connect(std::shared_ptr<const broker_config> config,
	std::shared_ptr<zmq::context_t> context,
	std::shared_ptr<worker_registry> router,
	std::shared_ptr<spdlog::logger> logger)
	: config_(config), logger_(logger), workers_(router), reactor_(context)
{
	if (logger_ == nullptr) {
		logger_ = helpers::create_null_logger();
	}

	auto clients_endpoint = "tcp://" + config_->get_client_address() + ":" + std::to_string(config_->get_client_port());
	logger_->debug() << "Binding clients to " + clients_endpoint;

	auto workers_endpoint = "tcp://" + config_->get_worker_address() + ":" + std::to_string(config_->get_worker_port());
	logger_->debug() << "Binding workers to " + workers_endpoint;

	auto monitor_endpoint =
		"tcp://" + config_->get_monitor_address() + ":" + std::to_string(config_->get_monitor_port());
	logger_->debug() << "Binding monitor to " + monitor_endpoint;

	reactor_.add_socket(KEY_WORKERS, std::make_shared<router_socket_wrapper>(context, workers_endpoint, true));
	reactor_.add_socket(KEY_CLIENTS, std::make_shared<router_socket_wrapper>(context, clients_endpoint, true));
	reactor_.add_socket(KEY_MONITOR, std::make_shared<router_socket_wrapper>(context, monitor_endpoint, false));

	reactor_.add_handler(
		{KEY_CLIENTS, KEY_WORKERS, KEY_TIMER}, std::make_shared<broker_handler>(config_, workers_, logger_));
	reactor_.add_async_handler(
		{KEY_STATUS_NOTIFIER}, std::make_shared<status_notifier_handler>(config_->get_notifier_config(), logger_));
}

void broker_connect::start_brokering()
{
	reactor_.start_loop();
	logger_->emerg() << "The main loop terminated";
}
