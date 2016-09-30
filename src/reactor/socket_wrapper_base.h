#ifndef RECODEX_BROKER_SOCKET_WRAPPER_BASE_HPP
#define RECODEX_BROKER_SOCKET_WRAPPER_BASE_HPP

#include <memory>
#include <zmq.hpp>

#include "message_container.h"

class socket_wrapper_base
{
protected:
	zmq::socket_t socket_;
	const std::string addr_;
	const bool bound_;

public:
	socket_wrapper_base(
		std::shared_ptr<zmq::context_t> context, zmq::socket_type type, const std::string &addr, const bool bound);

	virtual ~socket_wrapper_base();

	zmq_pollitem_t get_pollitem();

	virtual void initialize();

	void restart();

	virtual bool send_message(const message_container &) = 0;

	virtual bool receive_message(message_container &) = 0;
};


#endif // RECODEX_BROKER_SOCKET_WRAPPER_BASE_HPP
