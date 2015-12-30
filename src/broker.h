#ifndef CODEX_BROKER_BROKER_HPP
#define CODEX_BROKER_BROKER_HPP


#include <zmq.hpp>
#include <memory>
#include <bitset>

#include "spdlog/spdlog.h"
#include "broker_config.h"
#include "task_router.h"

struct message_receiver {
	enum type {
		CLIENT = 0,
		WORKER = 1
	};

	typedef std::bitset<2> set;
};

template <typename proxy>
class broker {
private:
	const broker_config &config_;

	std::shared_ptr<proxy> sockets_;

	task_router router_;
	std::shared_ptr<spdlog::logger> logger_;

	void process_worker_init (const std::string &id, zmq::message_t &message)
	{
		task_router::headers_t headers;

		while (message.more()) {
			sockets_->recv_workers(message);
			std::string header((char *) message.data(), message.size());

			size_t pos = header.find('=');
			size_t value_size = header.size() - (pos + 1);

			headers.emplace(header.substr(0, pos), header.substr(pos + 1, value_size));
		}

		router_.add_worker(task_router::worker_ptr(new worker(id, headers)));
	}

	void process_client_eval (const std::string &identity, zmq::message_t &message)
	{
		task_router::headers_t headers;
		std::string response("reject");

		sockets_->recv_clients(message);
		std::string job_id((char *) message.data(), message.size());

		// Load headers terminated by an empty frame
		while (true) {
			sockets_->recv_clients(message);

			// End of headers
			if (message.size() == 0) {
				break;
			}

			// Unexpected end of message - do nothing and return
			if (!message.more()) {
				return;
			}

			// Parse header, save it and continue
			std::string header((char *) message.data(), message.size());

			size_t pos = header.find('=');
			size_t value_size = header.size() - (pos + 1);

			headers.emplace(header.substr(0, pos), header.substr(pos + 1, value_size));
		}

		task_router::worker_ptr worker = router_.find_worker(headers);

		if (worker != nullptr) {
			response = "accept";

			// The first sent frame specifies the identity of the worker
			sockets_->send_workers_id(worker->identity);

			// Send the "eval" command
			sockets_->send_workers("eval", 4, ZMQ_SNDMORE);

			// Send the Job ID
			sockets_->send_workers(
				job_id.c_str(),
				job_id.length(),
				ZMQ_SNDMORE
			);

			// Forward remaining messages to the worker without actually understanding them
			while (message.more()) {
				sockets_->recv_clients(message);
				sockets_->send_workers(
					message.data(),
					message.size(),
					message.more() ? ZMQ_SNDMORE : 0
				);
			}
		} else {
			// Receive and ignore any remaining messages
			while (message.more()) {
				sockets_->recv_clients(message);
			}
		}

		sockets_->send_clients_id(identity);
		sockets_->send_clients(response.c_str(), response.length(), 0);
	}
public:
	broker (
		const broker_config &config,
		std::shared_ptr<proxy> sockets,
		std::shared_ptr<spdlog::logger> logger = nullptr
	) : config_(config), sockets_(sockets)
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

	void start_brokering ()
	{
		bool terminate = false;

		logger_->debug() << "Binding clients to tcp://*:" + std::to_string(config_.get_client_port());
		logger_->debug() << "Binding workers to tcp://*:" + std::to_string(config_.get_worker_port());

		sockets_->bind(
			std::string("tcp://*:") + std::to_string(config_.get_client_port()),
			std::string("tcp://*:") + std::to_string(config_.get_worker_port())
		);

		while (!terminate) {
			zmq::message_t message;
			message_receiver::set result;

			try {
				sockets_->poll(result, -1);
			} catch (zmq::error_t &e) {
				logger_->emerg() << "Terminating to poll... " << e.what();
				terminate = true;
			}

			// Received a message from the frontend
			if (result.test(message_receiver::type::CLIENT)) {
				message.rebuild();

				std::string identity;
				sockets_->recv_clients_id(message, identity);

				sockets_->recv_clients(message);
				std::string type((char*) message.data(), message.size());

				logger_->debug() << "Received message '" << type << "' from frontend";

				if (type == "eval") {
					process_client_eval(identity, message);
				}
			}

			// Received a message from the backend
			if (result.test(message_receiver::type::WORKER)) {
				message.rebuild();

				std::string identity;
				sockets_->recv_workers_id(message, identity);

				sockets_->recv_workers(message);
				std::string type((char *) message.data(), message.size());

				logger_->debug() << "Received message '" << type << "' from backend";

				if (type == "init") {
					process_worker_init(identity, message);
				}
			}

		}
	}
};


#endif //CODEX_BROKER_BROKER_HPP
