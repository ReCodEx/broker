#include "router_socket_wrapper.h"

router_socket_wrapper::router_socket_wrapper(
	std::shared_ptr<zmq::context_t> context, const std::string &addr, const bool bound)
	: socket_wrapper_base(context, zmq::socket_type::router, addr, bound)
{
}

bool router_socket_wrapper::send_message(const message_container &source)
{

	try {
		socket_.send(source.identity.c_str(), source.identity.size(), ZMQ_SNDMORE);
	} catch (const zmq::error_t &) {
		return false;
	}


	for (auto it = std::begin(source.data); it != std::end(source.data); ++it) {
		try {
			socket_.send(it->c_str(), it->size(), std::next(it) != std::end(source.data) ? ZMQ_SNDMORE : 0);
		} catch (const zmq::error_t &) {
			return false;
		}
	}

	return true;
}

bool router_socket_wrapper::receive_message(message_container &target)
{
	zmq::message_t msg;
	target.data.clear();

	try {
		socket_.recv(&msg, 0);
	} catch (const zmq::error_t &) {
		return false;
	}

	target.identity = std::string(static_cast<char *>(msg.data()), msg.size());

	while (msg.more()) {
		try {
			socket_.recv(&msg, 0);
		} catch (const zmq::error_t &) {
			return false;
		}

		target.data.emplace_back(static_cast<char *>(msg.data()), msg.size());
	}

	return true;
}
