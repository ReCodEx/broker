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
	void poll (message_receiver::set &result, int timeout, bool *terminate = nullptr)
	{
		result.reset();

		try {
			zmq::poll(items_, 2, timeout);
		} catch (zmq::error_t) {
			if (terminate != nullptr) {
				*terminate = true;
				return;
			}
		}

		if (items_[0].revents & ZMQ_POLLIN) {
			result.set(message_receiver::CLIENT, true);
		}

		if (items_[1].revents & ZMQ_POLLIN) {
			result.set(message_receiver::WORKER, true);
		}
	}

	/**
	 * Receive a message frame from the worker socket
	 */
	bool recv_workers (std::string &identity, std::vector<std::string> &target, bool *terminate = nullptr)
	{
		zmq::message_t msg;
		target.clear();
		bool retval;

		retval = workers_.recv(&msg, 0);

		if (!retval) {
			return false;
		}

		identity = std::string(static_cast<char *>(msg.data()), msg.size());

		while (msg.more()) {
			try {
				retval = workers_.recv(&msg, 0);
			} catch (zmq::error_t) {
				if (terminate != nullptr) {
					*terminate = true;
				}
				retval = false;
			}

			if (!retval) {
				return false;
			}

			target.emplace_back(static_cast<char *>(msg.data()), msg.size());
		}

		return true;
	}

	/**
	 * Receive a message frame from the client socket
	 */
	bool recv_clients (std::string &identity, std::vector<std::string> &target, bool *terminate = nullptr)
	{
		zmq::message_t msg;
		target.clear();
		bool retval;

		retval = clients_.recv(&msg, 0);

		if (!retval) {
			return false;
		}

		identity = std::string(static_cast<char *>(msg.data()), msg.size());

		retval = clients_.recv(&msg, 0); // Empty frame

		if (!retval) {
			return false;
		}

		while (msg.more()) {
			try {
				retval = clients_.recv(&msg, 0);
			} catch (zmq::error_t) {
				if (terminate != nullptr) {
					*terminate = true;
				}
				retval = false;
			}

			if (!retval) {
				return false;
			}

			target.emplace_back(static_cast<char *>(msg.data()), msg.size());
		}

		return true;
	}

	/**
	 * Send a message through the worker socket
	 */
	bool send_workers (const std::string &identity, const std::vector<std::string> &msg)
	{
		bool retval;
		retval = workers_.send(identity.c_str(), identity.size(), ZMQ_SNDMORE) >= 0;

		if (!retval) {
			return false;
		}

		for (auto it = std::begin(msg); it != std::end(msg); ++it) {
			retval = workers_.send(
				it->c_str(),
				it->size(),
				std::next(it) != std::end(msg) ? ZMQ_SNDMORE : 0
			) >= 0;

			if (!retval) {
				return false;
			}
		}

		return true;
	}

	/**
	 * Send a message through the client socket
	 */
	bool send_clients (const std::string &identity, const std::vector<std::string> &msg)
	{
		bool retval;
		retval = clients_.send(identity.c_str(), identity.size(), ZMQ_SNDMORE) >= 0;

		if (!retval) {
			return false;
		}

		retval = clients_.send("", 0, ZMQ_SNDMORE) >= 0; // Empty frame

		if (!retval) {
			return false;
		}

		for (auto it = std::begin(msg); it != std::end(msg); ++it) {
			retval = clients_.send(
				it->c_str(),
				it->size(),
				std::next(it) != std::end(msg) ? ZMQ_SNDMORE : 0
			) >= 0;

			if (!retval) {
				return false;
			}
		}

		return true;
	}
};

#endif //CODEX_BROKER_CONNECTION_PROXY_H
