#ifndef RECODEX_BROKER_REACTOR_H
#define RECODEX_BROKER_REACTOR_H

#include <atomic>
#include <map>
#include <memory>
#include <zmq.hpp>

#include "handler_interface.h"
#include "message_container.h"
#include "socket_wrapper_base.h"

/* Forward */
class reactor;

class handler_wrapper
{
public:
	handler_wrapper(reactor &reactor_ref, std::shared_ptr<handler_interface>);

	virtual ~handler_wrapper();

	/**
	 * Pass a message to the handler by calling it directly
	 * @param origin
	 * @param data
	 */
	virtual void operator()(const message_container &message);

protected:
	std::shared_ptr<handler_interface> handler_;
	reactor &reactor_;
};

class asynchronous_handler_wrapper : public handler_wrapper
{
public:
	asynchronous_handler_wrapper(zmq::context_t &context,
		zmq::socket_t &async_handler_socket,
		reactor &reactor_ref,
		std::shared_ptr<handler_interface>);
	~asynchronous_handler_wrapper();
	virtual void operator()(const message_container &message);

private:
	/**
	 * A message used to signal that the worker thread should terminate
	 */
	static const std::string TERMINATE_MSG;

	/**
	 * A ZeroMQ context
	 */
	zmq::context_t &context_;

	/**
	 * A reference to the socket used by the reactor to communicate with asynchronous handlers.
	 * It can only be used by the main reactor thread (i.e. not by the worker thread)
	 */
	zmq::socket_t &reactor_socket_;

	/**
	 * A reference to the worker thread
	 */
	std::thread worker_;

	/**
	 * The worker thread function
	 */
	void handler_thread();

	/**
	 * Send a message back to the reactor through the inprocess socket
	 */
	void send_response(zmq::socket_t &socket, const message_container &message);

	/**
	 * A unique identifier of the handler used in inprocess communication with the reactor
	 */
	const std::string unique_id_;
};

class reactor
{
public:
	/**
	 * An origin identifier for messages about elapsed time
	 */
	const static std::string KEY_TIMER;

	/**
	 * A unique identifier for the asynchronous handler socket
	 */
	const std::string unique_id;

	reactor(std::shared_ptr<zmq::context_t> context);

	/**
	 * Add a socket to be polled by the reactor
	 * @param socket
	 */
	void add_socket(const std::string &name, std::shared_ptr<socket_wrapper_base> socket);

	/**
	 * Add a handler for messages from given origins that is invoked directly
	 * (reactor waits for its completion, any calls to a response callback are processed immediately).
	 * @param origins subscribed origins
	 * @param handler
	 */
	void add_handler(const std::vector<std::string> &origins, std::shared_ptr<handler_interface> handler);

	/**
	 * Add a handler for messages from given origins that is invoked asynchronously
	 * (messages are passed to it through an in-process socket pair, which is also used by the response callback).
	 * @param origins
	 * @param handler
	 */
	void add_async_handler(const std::vector<std::string> &origins, std::shared_ptr<handler_interface> handler);

	/**
	 * Send a message through one of the sockets.
	 * This method is mostly called indirectly by the handlers.
	 * @param destination string identifier of the socket that should be used for sending the message
	 * @param message frames of the message
	 */
	void send_message(const message_container &message);

	/**
	 * Pass a message received from one of the sockets to the handlers.
	 * Public for easier testing and extensibility.
	 * @param origin string identifier of the socket from which the message came
	 * @param message frames of the message
	 */
	void process_message(const message_container &message);

	/**
	 * Start polling the underlying sockets and invoking handlers if needed.
	 * Handlers that need to keep track of elapsed time should subscribe to messages
	 * from ORIGIN_TIMER - it will notify them periodically.
	 */
	void start_loop();

	/**
	 * Tell the reactor to terminate as soon as possible.
	 * This method is thread-safe.
	 */
	void terminate();

private:
	std::map<std::string, std::shared_ptr<socket_wrapper_base>> sockets_;
	std::multimap<std::string, std::shared_ptr<handler_wrapper>> handlers_;
	std::shared_ptr<zmq::context_t> context_;

	/**
	 * An in-process socket used to communicate with asynchronous handlers
	 */
	zmq::socket_t async_handler_socket_;

	std::atomic<bool> termination_flag_;
};

#endif // RECODEX_BROKER_REACTOR_H
