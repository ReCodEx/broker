#ifndef CODEX_BROKER_CONNECTION_PROXY_H
#define CODEX_BROKER_CONNECTION_PROXY_H

#include <zmq.hpp>

#include "broker_connect.h"

/**
 * A trivial wrapper for the ZeroMQ sockets used by the broker,
 * whose purpose is to facilitate testing of the broker class
 */
class connection_proxy
{
private:
	static const size_t sockets_count_ = 3;
	zmq::context_t context_;
	zmq::socket_t clients_;
	zmq::socket_t workers_;
	zmq::socket_t monitor_;
	zmq_pollitem_t items_[sockets_count_];

public:
	connection_proxy()
		: context_(1), clients_(context_, ZMQ_ROUTER), workers_(context_, ZMQ_ROUTER), monitor_(context_, ZMQ_ROUTER)
	{
		items_[0].socket = (void *) clients_;
		items_[0].fd = 0;
		items_[0].events = ZMQ_POLLIN;
		items_[0].revents = 0;

		items_[1].socket = (void *) workers_;
		items_[1].fd = 0;
		items_[1].events = ZMQ_POLLIN;
		items_[1].revents = 0;

		items_[2].socket = (void *) monitor_;
		items_[2].fd = 0;
		items_[2].events = ZMQ_POLLIN;
		items_[2].revents = 0;
	}

	/**
	 * Bind the sockets to given addresses
	 */
	void set_addresses(
		const std::string &clients_addr, const std::string &workers_addr, const std::string &monitor_addr)
	{
		clients_.bind(clients_addr);
		workers_.bind(workers_addr);
		monitor_.connect(monitor_addr); // TODO:
	}

	/**
	 * Block execution until a message arrives to either socket.
	 * @param result If a message arrived to a socket, the corresponding bit field is set to true
	 * @param timeout The maximum time this function is allowed to block execution
	 * @param terminate If the underlying connection is terminated, the referenced variable is set to true
	 * @param elapsed_time Set to the time spent polling on success
	 */
	void poll(message_origin::set &result,
		std::chrono::milliseconds timeout,
		bool &terminate,
		std::chrono::milliseconds &elapsed_time)
	{
		result.reset();

		try {
			auto time_before_poll = std::chrono::system_clock::now();
			zmq::poll(items_, sockets_count_, timeout);
			auto time_after_poll = std::chrono::system_clock::now();

			elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(time_after_poll - time_before_poll);
		} catch (zmq::error_t) {
			terminate = true;
			return;
		}

		if (items_[0].revents & ZMQ_POLLIN) {
			result.set(message_origin::CLIENT, true);
		}

		if (items_[1].revents & ZMQ_POLLIN) {
			result.set(message_origin::WORKER, true);
		}

		if (items_[2].revents & ZMQ_POLLIN) {
			result.set(message_origin::MONITOR, true);
		}
	}

	/**
	 * Receive a message frame from the worker socket
	 */
	bool recv_workers(std::string &identity, std::vector<std::string> &target, bool *terminate = nullptr)
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
	bool recv_clients(std::string &identity, std::vector<std::string> &target, bool *terminate = nullptr)
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
	bool send_workers(const std::string &identity, const std::vector<std::string> &msg)
	{
		bool retval;
		retval = workers_.send(identity.c_str(), identity.size(), ZMQ_SNDMORE) >= 0;

		if (!retval) {
			return false;
		}

		for (auto it = std::begin(msg); it != std::end(msg); ++it) {
			retval = workers_.send(it->c_str(), it->size(), std::next(it) != std::end(msg) ? ZMQ_SNDMORE : 0) >= 0;

			if (!retval) {
				return false;
			}
		}

		return true;
	}

	/**
	 * Send a message through the client socket
	 */
	bool send_clients(const std::string &identity, const std::vector<std::string> &msg)
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
			retval = clients_.send(it->c_str(), it->size(), std::next(it) != std::end(msg) ? ZMQ_SNDMORE : 0) >= 0;

			if (!retval) {
				return false;
			}
		}

		return true;
	}

	/**
	 * Send a message through monitor socket
	 */
	bool send_monitor(const std::string &channel, const std::string &msg)
	{
		bool retval;
		const std::string monitor_socket_id = "recodex-monitor";

		retval = monitor_.send(monitor_socket_id.c_str(), monitor_socket_id.size(), ZMQ_SNDMORE) >= 0;
		if (!retval) {
			return false;
		}

		retval = monitor_.send(channel.c_str(), channel.size(), ZMQ_SNDMORE) >= 0;
		if (!retval) {
			return false;
		}

		retval = monitor_.send(msg.c_str(), msg.size(), 0) >= 0;
		return retval;
	}
};

#endif // CODEX_BROKER_CONNECTION_PROXY_H
