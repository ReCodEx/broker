#ifndef RECODEX_BROKER_BROKER_HPP
#define RECODEX_BROKER_BROKER_HPP


#include "commands/client_commands.h"
#include "commands/command_holder.h"
#include "commands/worker_commands.h"
#include "config/broker_config.h"
#include "helpers/logger.h"
#include "helpers/string_to_hex.h"
#include "notifier/empty_status_notifier.h"
#include "notifier/status_notifier.h"
#include "worker_registry.h"
#include <bitset>
#include <chrono>
#include <memory>

/**
 * Contains type definitions used by the proxy poll function.
 */
struct message_origin {
	/** Types of used sockets (by destination). */
	enum type { CLIENT = 0, WORKER = 1, MONITOR = 2 };

	/** Type used for describing which socket has a message while polling. */
	typedef std::bitset<3> set;
};

/**
 * Receives requests from clients and forwards them to correct workers.
 */
template <typename proxy> class broker_connect
{
private:
	/** Loaded broker configuration. */
	std::shared_ptr<const broker_config> config_;
	/** Thin wrapper around all connected sockets. */
	std::shared_ptr<proxy> sockets_;
	/** Command handlers for messages from workers. */
	std::shared_ptr<command_holder<proxy>> worker_cmds_;
	/** Command handlers for messages from clients (fronted). */
	std::shared_ptr<command_holder<proxy>> client_cmds_;
	/** System logger. */
	std::shared_ptr<spdlog::logger> logger_;
	/** Registry of connected and alive workers. */
	std::shared_ptr<worker_registry> workers_;
	/** Notifier which can be used to indicate frontend issues or some "good to know messages" */
	std::shared_ptr<status_notifier_interface> status_notifier_;

	/**
	 * Remove an expired worker. Also try to reassign the requests it was processing.
	 * @param expired_worker Pointer to expired worker.
	 */
	void remove_worker(worker_registry::worker_ptr expired_worker)
	{
		logger_->notice() << "Worker " + helpers::string_to_hex(expired_worker->identity) + " expired";

		workers_->remove_worker(expired_worker);
		auto requests = expired_worker->terminate();
		std::vector<worker::request_ptr> unassigned_requests;

		for (auto request : *requests) {
			worker_registry::worker_ptr substitute_worker = workers_->find_worker(request->headers);

			if (substitute_worker != nullptr) {
				substitute_worker->enqueue_request(request);

				if (substitute_worker->next_request()) {
					sockets_->send_workers(
						substitute_worker->identity, substitute_worker->get_current_request()->data.get());
				}
			} else {
				unassigned_requests.push_back(request);
			}
		}

		// there are requests which cannot be assigned, notify frontend about it
		if (!unassigned_requests.empty()) {
			std::string worker_id = helpers::string_to_hex(expired_worker->identity);
			std::vector<std::string> job_ids;
			for (auto request : unassigned_requests) {
				job_ids.push_back(request->data.get_job_id());
			}

			status_notifier_->rejected_jobs(job_ids, "Worker " + worker_id + " dieded");
		}
	}

public:
	/**
	 * Constructor with initialization.
	 * @param config Loaded broker configuration from config file.
	 * @param sockets Instance of socket wrapper (now @ref connection_proxy).
	 * @param router Instance of class managing connected workers and routing jobs to them.
	 * @param notifier Endpoint for reporting unexpected errors back to frontend.
	 * @param logger System logger.
	 */
	broker_connect(std::shared_ptr<const broker_config> config,
		std::shared_ptr<proxy> sockets,
		std::shared_ptr<worker_registry> router,
		std::shared_ptr<status_notifier_interface> notifier = nullptr,
		std::shared_ptr<spdlog::logger> logger = nullptr)
		: config_(config), sockets_(sockets), status_notifier_(notifier), logger_(logger), workers_(router)
	{
		if (logger_ == nullptr) {
			logger_ = helpers::create_null_logger();
		}

		if (status_notifier_ == nullptr) {
			status_notifier_ = std::make_shared<empty_status_notifier>();
		}

		// init worker commands
		worker_cmds_ = std::make_shared<command_holder<proxy>>(sockets_, router, status_notifier_, logger_);
		worker_cmds_->register_command("init", worker_commands::process_init<proxy>);
		worker_cmds_->register_command("done", worker_commands::process_done<proxy>);
		worker_cmds_->register_command("ping", worker_commands::process_ping<proxy>);
		worker_cmds_->register_command("progress", worker_commands::process_progress<proxy>);

		// init client commands
		client_cmds_ = std::make_shared<command_holder<proxy>>(sockets_, router, status_notifier_, logger_);
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
		logger_->debug() << "Binding clients to " + clients_endpoint;

		auto workers_endpoint =
			"tcp://" + config_->get_worker_address() + ":" + std::to_string(config_->get_worker_port());
		logger_->debug() << "Binding workers to " + workers_endpoint;

		auto monitor_endpoint =
			"tcp://" + config_->get_monitor_address() + ":" + std::to_string(config_->get_monitor_port());
		logger_->debug() << "Binding monitor to " + monitor_endpoint;

		sockets_->set_addresses(clients_endpoint, workers_endpoint, monitor_endpoint);


		const std::chrono::milliseconds ping_interval = config_->get_worker_ping_interval();
		const size_t max_liveness = config_->get_max_worker_liveness();
		std::chrono::milliseconds poll_limit = ping_interval;

		while (true) {
			bool terminate = false;
			message_origin::set result;
			std::chrono::milliseconds poll_duration(0);

			sockets_->poll(result, poll_limit, terminate, poll_duration);

			// Check if we spent enough time polling to decrease the workers liveness
			if (poll_duration >= poll_limit) {
				// Reset the time limit
				poll_limit = ping_interval;

				// Decrease liveness but don't clean up dead workers yet (in case of miracles)
				for (auto worker : workers_->get_workers()) {
					worker->liveness -= 1;
				}
			} else {
				poll_limit -= std::max(std::chrono::milliseconds(0), poll_duration);
			}

			if (terminate) {
				break;
			}

			// Log errors during message processing and command evaluation, but
			// don't let the broker die.
			try {
				// Received a message from the frontend
				if (result.test(message_origin::CLIENT)) {
					std::string identity;
					std::vector<std::string> message;

					sockets_->recv_clients(identity, message, &terminate);
					if (terminate) {
						break;
					}

					// Get command and remove it from its arguments
					std::string type = message.front();
					message.erase(message.begin());

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

					// Get command and remove it from its arguments
					std::string type = message.front();
					message.erase(message.begin());

					// ! Receive message logging was moved to particular worker commands callbacks.
					worker_cmds_->call_function(type, identity, message);

					// An incoming message means the worker is alive
					auto worker = workers_->find_worker_by_identity(identity);
					if (worker != nullptr) {
						worker->liveness = max_liveness;
					}
				}

				// Received a message from the monitor
				if (result.test(message_origin::MONITOR)) {
					// this should not happen
					logger_->error() << "Received message from monitor, but none expected.";
				}
			} catch (std::exception &e) {
				logger_->error() << "Unexpected exception during evaluation: " << e.what();
			}

			// Handle dead workers
			std::list<worker_registry::worker_ptr> to_remove;

			for (auto worker : workers_->get_workers()) {
				if (worker->liveness == 0) {
					to_remove.push_back(worker);
				}
			}

			for (auto worker : to_remove) {
				remove_worker(worker);
			}
		}

		logger_->emerg() << "Terminating to poll... ";
	}
};


#endif // RECODEX_BROKER_BROKER_HPP
