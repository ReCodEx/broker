#include "socket_wrapper_base.h"


socket_wrapper_base::socket_wrapper_base(
	std::shared_ptr<zmq::context_t> context, zmq::socket_type type, const std::string &addr, const bool bound)
	: socket_(*context, type), addr_(addr), bound_(bound)
{
	socket_.setsockopt(ZMQ_LINGER, 0);
}

socket_wrapper_base::~socket_wrapper_base()
{
}

zmq_pollitem_t socket_wrapper_base::get_pollitem()
{
	return zmq_pollitem_t{.socket = (void *) socket_, .fd = 0, .events = ZMQ_POLLIN, .revents = 0};
}

void socket_wrapper_base::restart()
{
	if (bound_) {
		socket_.unbind(addr_);
	} else {
		socket_.disconnect(addr_);
	}

	initialize();
}

void socket_wrapper_base::initialize()
{
	if (bound_) {
		socket_.bind(addr_);
	} else {
		socket_.connect(addr_);
	}
}
