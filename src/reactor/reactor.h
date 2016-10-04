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

/**
 * A basic wrapper for reactor event handlers, which just calls the handler callback directly
 * This class is meant to be extended to provide more complicated calling strategies (e.g. async callbacks).
 */
class handler_wrapper
{
public:
	/**
	 * @param reactor_ref reactor object which owns the handler
	 * @param handler the handler object
	 */
	handler_wrapper(reactor &reactor_ref, std::shared_ptr<handler_interface> handler);

	/**
	 * A virtual destructor
	 */
	virtual ~handler_wrapper();

	/**
	 * Pass a message to the handler by calling it directly
	 * @param message The message to be passed
	 */
	virtual void operator()(const message_container &message);

protected:
	/** The wrapped handler */
	std::shared_ptr<handler_interface> handler_;

	/** The reactor that owns the wrapper */
	reactor &reactor_;
};

/**
 * A wrapper for reactor event handlers which calls the handler asynchronously, using ZeroMQ inprocess sockets.
 * The handler itself runs in its own (exactly one) thread.
 */
class asynchronous_handler_wrapper : public handler_wrapper
{
public:
	/**
	 * @param context ZeroMQ context used to communicate with the reactor
	 * @param async_handler_socket The socket used to communicate with our worker thread (the reactor side)
	 * @param reactor_ref The reactor whcih owns the handler
	 * @param handler The handler object
	 */
	asynchronous_handler_wrapper(zmq::context_t &context,
		zmq::socket_t &async_handler_socket,
		reactor &reactor_ref,
		std::shared_ptr<handler_interface> handler);

	/**
	 * Destroy the handler by sending a termination message to the worker thread.
	 * The ZeroMQ connection must still work for this to function.
	 */
	~asynchronous_handler_wrapper();

	/**
	 * Pass a message to the handler asynchronously.
	 * The message is copied when being sent through the inprocess socket.
	 * @param message The message to be passed
	 */
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
	 * The worker thread function. It contains a loop that doesn't terminate until a termination message arrives
	 * from the reactor.
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

/**
 * Provides an event-based API for ZeroMQ network communication. Messages are transferred through registered sockets
 * and processed by handlers who don't have to use ZeroMQ directly. Running the handlers asynchronously is also
 * supported.
 */
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

	/**
	 * @param context A ZeroMQ context used to create sockets for asynchronous communication
	 */
	reactor(std::shared_ptr<zmq::context_t> context);

	/**
	 * Add a socket to be polled by the reactor
	 * @param name a name used as the key for messages transmitted through the socket
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
	 * @param message frames of the message
	 */
	void send_message(const message_container &message);

	/**
	 * Pass a message received from one of the sockets to the handlers.
	 * Public for easier testing and extensibility.
	 * @param message frames of the message
	 */
	void process_message(const message_container &message);

	/**
	 * Start polling the underlying sockets and invoking handlers if needed.
	 * Handlers that need to keep track of elapsed time should subscribe to messages
	 * from ORIGIN_TIMER - it will notify them periodically.
	 *
	 * The loop can be interrupted using the @a terminate method. When this happens, all handlers will be
	 * destroyed.
	 */
	void start_loop();

	/**
	 * Tell the reactor to terminate as soon as possible.
	 * This method is thread-safe.
	 */
	void terminate();

private:
	/**
	 * Sockets indexed by their keys
	 */
	std::map<std::string, std::shared_ptr<socket_wrapper_base>> sockets_;

	/**
	 * Handlers indexed by the keys of all the events they are subscribed to
	 */
	std::multimap<std::string, std::shared_ptr<handler_wrapper>> handlers_;

	/**
	 * A ZeroMQ context
	 */
	std::shared_ptr<zmq::context_t> context_;

	/**
	 * An in-process socket used to communicate with asynchronous handlers
	 */
	zmq::socket_t async_handler_socket_;

	/**
	 * Flag used to tell the reactor to terminate the main loop
	 */
	std::atomic<bool> termination_flag_;
};

#endif // RECODEX_BROKER_REACTOR_H
