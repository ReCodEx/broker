#ifndef RECODEX_BROKER_SOCKET_WRAPPER_BASE_HPP
#define RECODEX_BROKER_SOCKET_WRAPPER_BASE_HPP

#include <memory>
#include <zmq.hpp>

#include "message_container.h"

/**
 * A wrapper for ZeroMQ sockets that also contains the address the socket should use, so that we can easily restart
 * the connection.
 * Socket wrappers should also hide all technical details of the underlying sockets, such as empty delimiter frames.
 */
class socket_wrapper_base
{
protected:
	/**
	 * The wrapped socket
	 */
	zmq::socket_t socket_;

	/**
	 * An address to connect/bind to
	 */
	const std::string addr_;

	/**
	 * True if the socket should bind to an address, false if it connects
	 */
	const bool bound_;

public:
	/**
	 * @param context A ZeroMQ context used to create the socket
	 * @param type Type of the socket
	 * @param addr Address used by the socket
	 * @param bound True if the socket should bind to an address, false if it connects
	 */
	socket_wrapper_base(
		std::shared_ptr<zmq::context_t> context, zmq::socket_type type, const std::string &addr, const bool bound);

	/**
	 * A virtual destructor
	 */
	virtual ~socket_wrapper_base() = default;

	/**
	 * Get the pollitem structure used to poll the wrapped socket
	 * @return A ZeroMQ poll item structure
	 */
	zmq_pollitem_t get_pollitem();

	/**
	 * Connect or bind the socket
	 */
	virtual void initialize();

	/**
	 * Destroy the wrapped socket and connect/bind again with a new one
	 */
	void restart();

	/**
	 * Send a message through the socket
	 * @return true on success, false otherwise
	 */
	virtual bool send_message(const message_container &) = 0;

	/**
	 * Receive a message from the socket
	 * @return true on success, false otherwise
	 */
	virtual bool receive_message(message_container &) = 0;
};


#endif // RECODEX_BROKER_SOCKET_WRAPPER_BASE_HPP
