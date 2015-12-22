#ifndef CODEX_BROKER_BROKER_HPP
#define CODEX_BROKER_BROKER_HPP


#include <zmq.hpp>
#include <memory>
#include "spdlog/spdlog.h"
#include "broker_config.h"
#include "task_router.h"

class broker {
private:
	const broker_config &config_;

	zmq::context_t context_;
	zmq::socket_t clients_;
	zmq::socket_t workers_;

	task_router router_;
	std::shared_ptr<spdlog::logger> logger_;

	void process_worker_init (const std::string &identity, zmq::message_t &message);
	void process_client_eval (const std::string &identity, zmq::message_t &message);
public:
	broker (const broker_config &config, std::shared_ptr<spdlog::logger> logger = nullptr);

	void start_brokering ();

};


#endif //CODEX_BROKER_BROKER_HPP
