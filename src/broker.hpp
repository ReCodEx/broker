#ifndef CODEX_BROKER_BROKER_HPP
#define CODEX_BROKER_BROKER_HPP


#include <zmq.hpp>

#include "broker_config.hpp"
#include "task_router.hpp"

class broker {
private:
	const broker_config &config;

	zmq::context_t context;
	zmq::socket_t clients;
	zmq::socket_t workers;

	task_router router;

	void process_worker_init (zmq::message_t &message);
public:
	broker (const broker_config &config);

	void start_brokering ();

};


#endif //CODEX_BROKER_BROKER_HPP
