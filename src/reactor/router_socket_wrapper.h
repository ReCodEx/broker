#ifndef RECODEX_BROKER_ROUTER_SOCKET_WRAPPER_H
#define RECODEX_BROKER_ROUTER_SOCKET_WRAPPER_H

#include <zmq.hpp>

#include "socket_wrapper_base.h"

/**
 * Wraps a ZeroMQ router socket
 */
class router_socket_wrapper : public socket_wrapper_base
{
public:
	/**
	 * @param context a ZeroMQ context
	 * @param addr address used by the socket
	 * @param bound true if the socket should bind, false if it connects
	 */
	router_socket_wrapper(std::shared_ptr<zmq::context_t> context, const std::string &addr, const bool bound);

	~router_socket_wrapper() override = default;

	bool send_message(const message_container &message) override;

	bool receive_message(message_container &target) override;
};

#endif // RECODEX_BROKER_ROUTER_SOCKET_WRAPPER_H
