#ifndef CODEX_BROKER_BROKER_HPP
#define CODEX_BROKER_BROKER_HPP


#include <memory>
#include <bitset>
#include <spdlog/spdlog.h>

#include "config/broker_config.h"
#include "task_router.h"
#include "commands/command_holder.h"
#include "commands/worker_commands.h"
#include "commands/client_commands.h"

/**
 * Contains type definitions used by the proxy poll function
 */
struct message_origin {
	enum type { CLIENT = 0, WORKER = 1 };

	typedef std::bitset<2> set;
};

/**
 * Receives requests from clients and forwards them to workers
 */
template <typename proxy> class broker_connect
{
private:
	std::shared_ptr<const broker_config> config_;
	std::shared_ptr<proxy> sockets_;
	std::shared_ptr<command_holder<proxy>> worker_cmds_;
	std::shared_ptr<command_holder<proxy>> client_cmds_;
	std::shared_ptr<spdlog::logger> logger_;

public:
	broker_connect(std::shared_ptr<const broker_config> config,
		std::shared_ptr<proxy> sockets,
		std::shared_ptr<task_router> router,
		std::shared_ptr<spdlog::logger> logger = nullptr)
		: config_(config), sockets_(sockets)
	{
		if (logger != nullptr) {
			logger_ = logger;
		} else {
			// Create logger manually to avoid global registration of logger
			auto sink = std::make_shared<spdlog::sinks::stderr_sink_st>();
			logger_ = std::make_shared<spdlog::logger>("broker_nolog", sink);
			// Set loglevel to 'off' cause no logging
			logger_->set_level(spdlog::level::off);
		}

		// init worker commands
		worker_cmds_ = std::make_shared<command_holder<proxy>>(sockets_, router, logger_);
		worker_cmds_->register_command("init", worker_commands::process_init<proxy>);
		worker_cmds_->register_command("done", worker_commands::process_done<proxy>);

		// init client commands
		client_cmds_ = std::make_shared<command_holder<proxy>>(sockets_, router, logger_);
		client_cmds_->register_command("eval", client_commands::process_eval<proxy>);
	}

	/**
	 * Bind to sockets and start receiving and routing requests.
	 * Blocks execution until the underlying ZeroMQ context is terminated.
	 */
	void start_brokering()
	{
		auto clients_endpoint =
			"tcp://" + config_->get_client_address() + ":" + std::to_string(config_->get_client_port());
		auto workers_endpoint =
			"tcp://" + config_->get_worker_address() + ":" + std::to_string(config_->get_worker_port());
		logger_->debug() << "Binding clients to " + clients_endpoint;
		logger_->debug() << "Binding workers to " + workers_endpoint;

		sockets_->bind(clients_endpoint, workers_endpoint);

		while (true) {
			bool terminate = false;
			message_origin::set result;

			sockets_->poll(result, -1, &terminate);

			if (terminate) {
				break;
			}

			// Received a message from the frontend
			if (result.test(message_origin::CLIENT)) {
				std::string identity;
				std::vector<std::string> message;

				sockets_->recv_clients(identity, message, &terminate);
				if (terminate) {
					break;
				}

				std::string &type = message.at(0);

				logger_->debug() << "Received message '" << type << "' from frontend";

				client_cmds_->call_function(type, identity, message);
			}

			// Received a message from the backend
			if (result.test(message_origin::WORKER)) {
				std::string identity;
				std::vector<std::string> message;

				sockets_->recv_workers(identity, message, &terminate);
				if (terminate) {
					break;
				}

				std::string &type = message.at(0);

				logger_->debug() << "Received message '" << type << "' from backend";

				worker_cmds_->call_function(type, identity, message);
			}
		}

		logger_->emerg() << "Terminating to poll... ";
	}
};


#endif // CODEX_BROKER_BROKER_HPP
