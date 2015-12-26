#ifndef CODEX_BROKER_CONNECTION_PROXY_H
#define CODEX_BROKER_CONNECTION_PROXY_H

#include <zmq.hpp>

#include "broker.h"

/**
 * A trivial wrapper for the ZeroMQ sockets used by the broker,
 * whose purpose is to facilitate testing of the broker class
 */
class connection_proxy {
private:
	zmq::context_t context_;
	zmq::socket_t clients_;
	zmq::socket_t workers_;
	zmq_pollitem_t items_[2];

public:
	connection_proxy () : context_(1), clients_(context_, ZMQ_ROUTER), workers_(context_, ZMQ_ROUTER)
	{
		items_[0].socket = (void *) clients_;
		items_[0].fd = 0;
		items_[0].events = ZMQ_POLLIN;
		items_[0].revents = 0;

		items_[1].socket = (void *) workers_;
		items_[1].fd = 0;
		items_[1].events = ZMQ_POLLIN;
		items_[1].revents = 0;
	}

	/**
	 * Bind the sockets to given addresses
	 */
	void bind (const std::string &clients_addr, const std::string &workers_addr)
	{
		clients_.bind(clients_addr);
		workers_.bind(workers_addr);
	}

	/**
	 * Block execution until a message arrives to either socket.
	 * @param result If a message arrived to a socket, the corresponding bit field is set to true
	 */
	void poll (message_receiver::set &result, int timeout)
	{
		result.reset();
		zmq::poll(items_, 2, timeout);

		if (items_[0].revents & ZMQ_POLLIN) {
			result.set(message_receiver::type::CLIENT, true);
		}

		if (items_[1].revents & ZMQ_POLLIN) {
			result.set(message_receiver::type::WORKER, true);
		}
	}

	/**
	 * Receive an identity string from a worker connection
	 */
	void recv_workers_id (zmq::message_t &msg, std::string &id)
	{
		workers_.recv(&msg, 0);
		id = std::string((char *) msg.data(), msg.size());
	}

	/**
	 * Receive a message frame from the worker socket
	 */
	void recv_workers (zmq::message_t &msg)
	{
		workers_.recv(&msg, 0);
	}

	/**
	 * Receive an identity string from a client connection
	 */
	void recv_clients_id (zmq::message_t &msg, std::string &id)
	{
		clients_.recv(&msg, 0);
		id = std::string((char *) msg.data(), msg.size());
		clients_.recv(&msg, 0);
	}

	/**
	 * Receive a message frame from the client socket
	 */
	void recv_clients (zmq::message_t &msg)
	{
		clients_.recv(&msg, 0);
	}

	/**
	 * Send an identity string to the worker socket.
	 * This is used to specify which worker should receive the following frames
	 */
	void send_workers_id (const std::string &id)
	{
		workers_.send(id.data(), id.size(), ZMQ_SNDMORE);
	}

	/**
	 * Send a message frame through the worker socket
	 */
	void send_workers (const void *data, size_t size, int flags)
	{
		workers_.send(data, size, flags);
	}

	/**
	 * Send an identity string to the client socket.
	 * This is used to specify which client should receive the following frames
	 */
	void send_clients_id (const std::string &id)
	{
		clients_.send(id.data(), id.size(), ZMQ_SNDMORE);
		clients_.send("", 0, ZMQ_SNDMORE);
	}

	/**
	 * Send a message frame through the client socket
	 */
	void send_clients (const void *data, size_t size, int flags)
	{
		clients_.send(data, size, flags);
	}
};

#endif //CODEX_BROKER_CONNECTION_PROXY_H
