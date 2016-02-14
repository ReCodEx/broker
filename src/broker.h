#ifndef CODEX_BROKER_BROKER_HPP
#define CODEX_BROKER_BROKER_HPP


#include <memory>
#include <bitset>
#include <spdlog/spdlog.h>

#include "broker_config.h"
#include "task_router.h"

/**
 * Contains type definitions used by the proxy poll function
 */
struct message_origin {
	enum type {
		CLIENT = 0,
		WORKER = 1
	};

	typedef std::bitset<2> set;
};

/**
 * Receives requests from clients and forwards them to workers
 */
template <typename proxy>
class broker {
private:
	std::shared_ptr<const broker_config> config_;
	std::shared_ptr<proxy> sockets_;
	std::shared_ptr<task_router> router_;
	std::shared_ptr<spdlog::logger> logger_;

	/**
	 * Process an "init" request from a worker.
	 * That means storing the identity and headers of the worker
	 * so that we can forward jobs to it.
	 */
	void process_worker_init (const std::string &identity, const std::vector<std::string> &message)
	{
		task_router::headers_t headers;

		for (auto it = std::begin(message) + 1; it != std::end(message); ++it) {
			auto &header = *it;
			size_t pos = header.find('=');
			size_t value_size = header.size() - (pos + 1);

			headers.emplace(header.substr(0, pos), header.substr(pos + 1, value_size));
		}

		router_->add_worker(task_router::worker_ptr(new worker(identity, headers)));
	}

	/**
	 * Process an "eval" request from a client
	 */
	void process_client_eval (const std::string &identity, const std::vector<std::string> &message)
	{
		std::string job_id = message.at(1);
		task_router::headers_t headers;

		// Load headers terminated by an empty frame
		auto it = std::begin(message) + 2;

		while (true) {
			// End of headers
			if (it->size() == 0) {
				++it;
				break;
			}

			// Unexpected end of message - do nothing and return
			if (std::next(it) == std::end(message)) {
				return;
			}

			// Parse header, save it and continue
			size_t pos = it->find('=');
			size_t value_size = it->size() - (pos + 1);

			headers.emplace(it->substr(0, pos), it->substr(pos + 1, value_size));
			 ++it;
		}

		task_router::worker_ptr worker = router_->find_worker(headers);

		if (worker != nullptr) {
			std::vector<std::string> request = {"eval", job_id};

			// Forward remaining messages to the worker without actually understanding them
			for (; it != std::end(message); ++it) {
				request.push_back(*it);
			}

			sockets_->send_workers(worker->identity, request);
			sockets_->send_clients(identity, std::vector<std::string>{"accept"});
		} else {
			sockets_->send_clients(identity, std::vector<std::string>{"reject"});
		}
	}

public:
	broker (
		std::shared_ptr<const broker_config> config,
		std::shared_ptr<proxy> sockets,
		std::shared_ptr<task_router> router,
		std::shared_ptr<spdlog::logger> logger = nullptr
	) : config_(config), sockets_(sockets), router_(router)
	{
		if (logger != nullptr) {
			logger_ = logger;
		} else {
			//Create logger manually to avoid global registration of logger
			auto sink = std::make_shared<spdlog::sinks::stderr_sink_st>();
			logger_ = std::make_shared<spdlog::logger>("broker_nolog", sink);
			//Set loglevel to 'off' cause no logging
			logger_->set_level(spdlog::level::off);
		}
	}

	/**
	 * Bind to sockets and start receiving and routing requests.
	 * Blocks execution until the underlying ZeroMQ context is terminated.
	 */
	void start_brokering ()
	{
		logger_->debug() << "Binding clients to tcp://*:" + std::to_string(config_->get_client_port());
		logger_->debug() << "Binding workers to tcp://*:" + std::to_string(config_->get_worker_port());

		sockets_->bind(
			std::string("tcp://*:") + std::to_string(config_->get_client_port()),
			std::string("tcp://*:") + std::to_string(config_->get_worker_port())
		);

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

				if (type == "eval") {
					process_client_eval(identity, message);
				}
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

				if (type == "init") {
					process_worker_init(identity, message);
				}
			}

		}

		logger_->emerg() << "Terminating to poll... ";
	}
};


#endif //CODEX_BROKER_BROKER_HPP
