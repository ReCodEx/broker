#ifndef RECODEX_BROKER_BROKER_HANDLER_H
#define RECODEX_BROKER_BROKER_HANDLER_H

#include <spdlog/logger.h>

#include "../config/broker_config.h"
#include "../notifier/status_notifier.h"
#include "../reactor/handler_interface.h"
#include "../worker_registry.h"

/**
 * Processes requests from workers and clients and forwards them accordingly.
 */
class broker_handler : public handler_interface
{
public:
	/**
	 * @param config broker configuration
	 * @param workers worker registry (it's acceptable if it already contains some workers)
	 * @param logger an optional logger
	 */
	broker_handler(std::shared_ptr<const broker_config> config,
		std::shared_ptr<worker_registry> workers,
		std::shared_ptr<spdlog::logger> logger);
	void on_request(const message_container &message, response_cb respond);

private:
	/** Broker configuration */
	std::shared_ptr<const broker_config> config_;

	/** Worker registry used for keeping track of workers and their jobs */
	std::shared_ptr<worker_registry> workers_;

	/** A system logger */
	std::shared_ptr<spdlog::logger> logger_;

	/** Time since we last heard from each worker or decreased their liveness */
	std::map<worker_registry::worker_ptr, std::chrono::milliseconds> worker_timers_;

	/**
	 * Process an "init" request from a worker.
	 * That means storing the identity and headers of the worker so that we can forward jobs to it.
	 */
	void process_worker_init(
		const message_container &message, response_cb respond, status_notifier_interface &status_notifier);

	/**
	 * Process a "done" message from a worker.
	 * We notify the frontend and if possible, assign a new job to the worker.
	 */
	void process_worker_done(
		const message_container &message, response_cb respond, status_notifier_interface &status_notifier);

	/**
	 * Process a "ping" message from worker.
	 * Find worker in registry and send him back "pong" message.
	 */
	void process_worker_ping(
		const message_container &message, response_cb respond, status_notifier_interface &status_notifier);

	/**
	 * Process a "state" message from worker.
	 * Resend "state" message to monitor service.
	 */
	void process_worker_progress(
		const message_container &message, response_cb respond, status_notifier_interface &status_notifier);

	/**
	 * Process an "eval" request from a client.
	 * Client requested evaluation, so hand it over to proper worker with corresponding headers.
	 * "accept" or "reject" message is send back to client.
	 */
	void process_client_eval(
		const message_container &message, response_cb respond, status_notifier_interface &status_notifier);

	/**
	 * Process a message about elapsed time from the reactor.
	 * If we haven't heard from a worker in a long time, we decrease its liveness counter. When this counter
	 * reaches zero, we consider it dead and try to reassign its jobs.
	 */
	void process_timer(
		const message_container &message, response_cb respond, status_notifier_interface &status_notifier);
};

#endif // RECODEX_BROKER_BROKER_HANDLER_H
