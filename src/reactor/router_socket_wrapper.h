#ifndef RECODEX_BROKER_ROUTER_SOCKET_WRAPPER_H
#define RECODEX_BROKER_ROUTER_SOCKET_WRAPPER_H

#include <zmq.hpp>

#include "socket_wrapper_base.h"

class router_socket_wrapper : public socket_wrapper_base
{
public:
	router_socket_wrapper(std::shared_ptr<zmq::context_t> context, const std::string &addr, const bool bound);

	bool send_message(const message_container &message);

	bool receive_message(message_container &target);
};

#endif // RECODEX_BROKER_ROUTER_SOCKET_WRAPPER_H
