#ifndef RECODEX_BROKER_BROKER_HPP
#define RECODEX_BROKER_BROKER_HPP


#include "config/broker_config.h"
#include "handlers/broker_handler.h"
#include "handlers/status_notifier_handler.h"
#include "helpers/logger.h"
#include "notifier/empty_status_notifier.h"
#include "notifier/status_notifier.h"
#include "reactor/command_holder.h"
#include "reactor/reactor.h"
#include "reactor/router_socket_wrapper.h"
#include "worker_registry.h"
#include <chrono>
#include <memory>

/**
 * Receives requests from clients and forwards them to correct workers.
 */
class broker_connect
{
private:
	/** Loaded broker configuration. */
	std::shared_ptr<const broker_config> config_;
	/** System logger. */
	std::shared_ptr<spdlog::logger> logger_;
	/** Registry of connected and alive workers. */
	std::shared_ptr<worker_registry> workers_;
	/** A reactor that provides us with an event-based API to communicate with the clients and workers */
	reactor reactor_;

public:
	/** A string key for the socket connected to the workers */
	const static std::string KEY_WORKERS;

	/** A string key for the socket connected to the clients */
	const static std::string KEY_CLIENTS;

	/** A string key for the socket connected to the monitor */
	const static std::string KEY_MONITOR;

	/** A string key for messages for the status notifier */
	const static std::string KEY_STATUS_NOTIFIER;

	/** A string key for messages about time elapsed in the poll loop */
	const static std::string KEY_TIMER;

	/** Identity of the monitor peer (necessary when working with router sockets) */
	const static std::string MONITOR_IDENTITY;

	/**
	 * @param config a configuration object used to set up the connections
	 * @param context ZeroMQ context
	 * @param router A registry used to track workers and their jobs
	 * @param logger
	 */
	broker_connect(std::shared_ptr<const broker_config> config,
		std::shared_ptr<zmq::context_t> context,
		std::shared_ptr<worker_registry> router,
		std::shared_ptr<spdlog::logger> logger = nullptr);

	/**
	 * Bind to sockets and start receiving and routing requests.
	 * Blocks execution until the underlying ZeroMQ context is terminated.
	 */
	void start_brokering();
};


#endif // RECODEX_BROKER_BROKER_HPP
