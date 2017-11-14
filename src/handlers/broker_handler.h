#ifndef RECODEX_BROKER_BROKER_HANDLER_H
#define RECODEX_BROKER_BROKER_HANDLER_H

#include <spdlog/logger.h>

#include "../config/broker_config.h"
#include "../notifier/status_notifier.h"
#include "../queuing/queue_manager_interface.h"
#include "../reactor/command_holder.h"
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
		std::shared_ptr<queue_manager_interface> queue,
		std::shared_ptr<spdlog::logger> logger);

	void on_request(const message_container &message, response_cb respond);

private:
	const std::string STATS_QUEUED_JOBS = "queued-jobs";

	const std::string STATS_EVALUATED_JOBS = "evaluated-jobs";

	const std::string STATS_JOBS_IN_PROGRESS = "jobs-in-progress";

	const std::string STATS_FAILED_JOBS = "failed-jobs";

	const std::string STATS_WORKER_COUNT = "worker-count";

	const std::string STATS_IDLE_WORKER_COUNT = "idle-worker-count";

	/** Broker configuration */
	std::shared_ptr<const broker_config> config_;

	/** Worker registry used for keeping track of workers and their jobs */
	std::shared_ptr<worker_registry> workers_;

	/** The queue manager */
	std::shared_ptr<queue_manager_interface> queue_;

	/** A system logger */
	std::shared_ptr<spdlog::logger> logger_;

	/** Time since we last heard from each worker or decreased their liveness */
	std::map<worker_registry::worker_ptr, std::chrono::milliseconds> worker_timers_;

	/** Various statistics */
	std::map<std::string, std::size_t> runtime_stats_;

	/** Handlers for commands received from the workers */
	command_holder worker_commands_;

	/** Handlers for commands received from the clients */
	command_holder client_commands_;

	bool is_frozen_ = false;

	/**
	 * Process an "init" request from a worker.
	 * That means storing the identity and headers of the worker so that we can forward jobs to it.
	 */
	void process_worker_init(const std::string &identity, const std::vector<std::string> &message, response_cb respond);

	/**
	 * Process a "done" message from a worker.
	 * We notify the frontend and if possible, assign a new job to the worker.
	 */
	void process_worker_done(const std::string &identity, const std::vector<std::string> &message, response_cb respond);

	/**
	 * Process a "ping" message from worker.
	 * Find worker in registry and send him back "pong" message.
	 */
	void process_worker_ping(const std::string &identity, const std::vector<std::string> &message, response_cb respond);

	/**
	 * Process a "state" message from worker.
	 * Resend "state" message to monitor service.
	 */
	void process_worker_progress(
		const std::string &identity, const std::vector<std::string> &message, response_cb respond);

	/**
	 * Process an "eval" request from a client.
	 * Client requested evaluation, so hand it over to proper worker with corresponding headers.
	 * "accept" or "reject" message is send back to client.
	 */
	void process_client_eval(const std::string &identity, const std::vector<std::string> &message, response_cb respond);

	/**
	 * Process a request for data about system load.
	 */
	void process_client_get_runtime_stats(
		const std::string &identity, const std::vector<std::string> &message, response_cb respond);

	/**
	 * Process a request to freeze the broker so that it doesn't accept any further requests
	 */
	void process_client_freeze(
		const std::string &identity, const std::vector<std::string> &message, response_cb respond);

	/**
	 * Process a request to unfreeze the broker so that it can accept requests once again
	 */
	void process_client_unfreeze(
		const std::string &identity, const std::vector<std::string> &message, response_cb respond);

	/**
	 * Process a message about elapsed time from the reactor.
	 * If we haven't heard from a worker in a long time, we decrease its liveness counter. When this counter
	 * reaches zero, we consider it dead and try to reassign its jobs.
	 */
	void process_timer(const message_container &message, response_cb respond);

	/**
	 * Find a substitute worker to try and process the request again.
	 * @param request the request to reassign
	 * @param respond a callback to notify the worker about the reassigned job
	 * @return true on success, false otherwise
	 */
	bool reassign_request(worker::request_ptr request, response_cb respond);

	/**
	 * Send a job to a worker
	 * @param worker the worker in need of a new job (it must be free)
	 * @param request the request
	 * @param respond a callback to notify the worker about the reassigned job
	 * @return true if a job was sent to the worker, false otherwise
	 */
	void send_request(worker_registry::worker_ptr worker, request_ptr request, response_cb respond);

	/**
	 * Check if a request can be reassigned one more time and notify the frontend if not.
	 * @param request the request to be checked
	 * @param status_notifier used to notify the frontend when the request doesn't pass the check
	 * @param respond a callback to notify the monitor in case of failure
	 * @param failure_msg a message describing the failure of the last request
	 * @return true if the request can be reassigned, false otherwise
	 */
	bool check_failure_count(worker::request_ptr request,
		status_notifier_interface &status_notifier,
		response_cb respond,
		const std::string &failure_msg);

	/**
	 * Notify the monitor about an error that might not have been reported by a worker
	 * @param request the request in which the error ocurred
	 * @param message the message to send to the monitor, e.g. FAILED or ABORTED
	 * @param respond a callback to notify the monitor
	 */
	void notify_monitor(worker::request_ptr request, const std::string &message, response_cb respond);

	/**
	 * Notify the monitor about an error that might not have been reported by a worker
	 * @param job_id an id of the job in which the error ocurred
	 * @param message the message to send to the monitor, e.g. FAILED or ABORTED
	 * @param respond a callback to notify the monitor
	 */
	void notify_monitor(const std::string &job_id, const std::string &message, response_cb respond);
};

#endif // RECODEX_BROKER_BROKER_HANDLER_H
