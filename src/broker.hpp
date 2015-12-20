#ifndef CODEX_BROKER_BROKER_HPP
#define CODEX_BROKER_BROKER_HPP


#include <zmq.hpp>
#include <memory>
#include "spdlog/spdlog.h"
#include "broker_config.hpp"
#include "task_router.hpp"

class broker {
private:
	const broker_config &config;

	zmq::context_t context;
	zmq::socket_t clients;
	zmq::socket_t workers;

	task_router router;
	std::shared_ptr<spdlog::logger> logger_;

	void process_worker_init (const std::string &identity, zmq::message_t &message);
public:
	broker (const broker_config &config);

	void start_brokering ();

};


#endif //CODEX_BROKER_BROKER_HPP
