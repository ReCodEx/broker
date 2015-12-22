#include "broker.h"

broker::broker (const broker_config &config, std::shared_ptr<spdlog::logger> logger):
	config_(config), context_(1), clients_(context_, ZMQ_ROUTER), workers_(context_, ZMQ_ROUTER), router_(logger)
{
	if(logger != nullptr) {
		logger_ = logger;
	} else {
		//Create logger manually to avoid global registration of logger
		auto sink = std::make_shared<spdlog::sinks::stderr_sink_st>();
		logger_ = std::make_shared<spdlog::logger>("broker_nolog", sink);
		//Set loglevel to 'off' cause no logging
		logger_->set_level(spdlog::level::off);
	}
}

void broker::start_brokering ()
{
	bool terminate = false;

	logger_->debug() << "Binding clients to tcp://*:" + std::to_string(config_.get_client_port());
	clients_.bind(std::string("tcp://*:") + std::to_string(config_.get_client_port()));
	logger_->debug() << "Binding workers to tcp://*:" + std::to_string(config_.get_worker_port());
	workers_.bind(std::string("tcp://*:") + std::to_string(config_.get_worker_port()));

	zmq_pollitem_t items[] = {
			{(void *) clients_, 0, ZMQ_POLLIN, 0},
			{(void *) workers_, 0, ZMQ_POLLIN, 0}
	};

	while (!terminate) {
		zmq::message_t message;

		try {
			zmq::poll(items, 2, -1);
		} catch (zmq::error_t &e) {
			logger_->emerg() << "Terminating to poll... " << e.what();
			terminate = true;
		}

		/* Received a message from the frontend */
		if (items[0].revents & ZMQ_POLLIN) {
			message.rebuild();

			clients_.recv(&message, 0);
			std::string identity((char*) message.data(), message.size());
			clients_.recv(&message, 0); // empty frame after identity

			clients_.recv(&message, 0);
			std::string type((char*) message.data(), message.size());

			logger_->debug() << "Received message '" << type << "' from frontend";

			if (type == "eval") {
				process_client_eval(identity, message);
			}

			while (message.more()) {
				clients_.recv(&message, 0);
			}
		}

		/* Received a message from the backend */
		if (items[1].revents & ZMQ_POLLIN) {
			message.rebuild();

			workers_.recv(&message, 0);
			std::string identity((char *) message.data(), message.size());

			workers_.recv(&message, 0);
			std::string type((char *) message.data(), message.size());

			logger_->debug() << "Received message '" << type << "' from backend";

			if (type == "init") {
				process_worker_init(identity, message);
			}

			while (message.more()) {
				workers_.recv(&message, 0);
			}
		}

	}
}

void broker::process_worker_init (const std::string &id, zmq::message_t &message)
{
	task_router::headers_t headers;

	while (message.more()) {
		workers_.recv(&message, 0);
		std::string header((char *) message.data(), message.size());

		size_t pos = header.find('=');
		size_t value_size = header.size() - (pos + 1);

		headers.emplace(header.substr(0, pos), header.substr(pos + 1, value_size));
	}

	router_.add_worker(task_router::worker_ptr(new worker(id, headers)));
}

void broker::process_client_eval (const std::string &identity, zmq::message_t &message)
{
	task_router::headers_t headers;
	std::string response("reject");

	while (message.more()) {
		clients_.recv(&message, 0);
		std::string header((char *) message.data(), message.size());

		size_t pos = header.find('=');
		size_t value_size = header.size() - (pos + 1);

		headers.emplace(header.substr(0, pos), header.substr(pos + 1, value_size));
	}

	task_router::worker_ptr worker = router_.find_worker(headers);

	if (worker != nullptr) {
		response = "accept";

		workers_.send(
			worker->identity.c_str(),
			worker->identity.length(),
			ZMQ_SNDMORE
		);
		workers_.send("", 0, ZMQ_SNDMORE);
		workers_.send("eval", 4, 0);
	}

	clients_.send((void *) identity.c_str(), identity.length(), ZMQ_SNDMORE);
	clients_.send((void *) "", 0, ZMQ_SNDMORE);
	clients_.send(response.c_str(), response.length(), 0);
}
