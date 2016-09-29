#ifndef RECODEX_BROKER_BROKER_HPP
#define RECODEX_BROKER_BROKER_HPP


#include "commands/command_holder.h"
#include "config/broker_config.h"
#include "handlers/broker_handler.h"
#include "handlers/status_notifier_handler.h"
#include "helpers/logger.h"
#include "notifier/empty_status_notifier.h"
#include "notifier/status_notifier.h"
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

	reactor reactor_;

public:
	const static std::string KEY_WORKERS;
	const static std::string KEY_CLIENTS;
	const static std::string KEY_MONITOR;
	const static std::string KEY_STATUS_NOTIFIER;
	const static std::string KEY_TIMER;
	/**
	 * Constructor with initialization.
	 * @param config Loaded broker configuration from config file.
	 * @param sockets Instance of socket wrapper (now @ref connection_proxy).
	 * @param router Instance of class managing connected workers and routing jobs to them.
	 * @param logger System logger.
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
