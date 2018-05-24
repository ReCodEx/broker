#ifndef RECODEX_BROKER_QUEUE_MANAGER_INTERFACE_HPP
#define RECODEX_BROKER_QUEUE_MANAGER_INTERFACE_HPP

#include "../worker.h"
#include "../worker_registry.h"

using worker_ptr = worker_registry::worker_ptr;
using request_ptr = worker::request_ptr;

/**
 * Describes the result of an enqueue operation
 */
struct enqueue_result {
	/**
	 * The worker to which the request was assigned (if any)
	 */
	worker_ptr assigned_to;

	/**
	 * True if the request was successfully enqueued or assigned to a worker, false otherwise
	 */
	bool enqueued;
};

class queue_manager_interface
{
public:
	/** Destructor */
	virtual ~queue_manager_interface() = default;

	/**
	 * Register a new worker. This can result into a job being assigned to it right away.
	 * Every worker used by the queue manager must be registered using this method.
	 * @param worker the new worker
	 * @param current_request the request that is currently being processed by the worker
	 * @return a newly assigned request (nullptr is returned when current_request is specified or when there is no
	 *         request to assign to the worker)
	 */
	virtual request_ptr add_worker(worker_ptr worker, request_ptr current_request = nullptr) = 0;

	/**
	 * Assign a queued request to given worker. If this succeeds, the request must be sent to the actual worker
	 * machine by the caller.
	 * @param worker
	 * @return a new request or nullptr if nothing can be assigned to the worker
	 */
	virtual request_ptr assign_request(worker_ptr worker) = 0;

	/**
	 * Remove a worker and return the request it was processing along with the requests that cannot be processed
	 * after its departure.
	 * This method is called when the worker is considered dead. It should not attempt to reassign any requests.
	 * @return requests that cannot be completed
	 */
	virtual std::shared_ptr<std::vector<request_ptr>> worker_terminated(worker_ptr) = 0;

	/**
	 * Try to enqueue a request. If it is assigned, it must be sent to the actual worker by the caller.
	 * @param request the request to be enqueued
	 * @return a structure containing details about the result
	 */
	virtual enqueue_result enqueue_request(request_ptr request) = 0;

	/**
	 * Get the total amount of queued requests
	 */
	virtual size_t get_queued_request_count() = 0;

	/**
	 * Get the request currently being processed by given worker
	 */
	virtual request_ptr get_current_request(worker_ptr worker) = 0;

	/**
	 * Mark the current request of a worker as complete and possibly assign it another request.
	 * Called when the actual worker machine finishes processing the request.
	 * @param worker the worker that finished its job
	 * @return a new request assigned to the worker (if any)
	 */
	virtual request_ptr worker_finished(worker_ptr worker) = 0;

	/**
	 * Mark the current request of a worker as cancelled and do not assign it another request.
	 * Called when the worker machine fails to process the request.
	 * The caller can decide whether the request should be enqueued again or not.
	 * @param worker the worker whose job was cancelled
	 * @return the cancelled request
	 */
	virtual request_ptr worker_cancelled(worker_ptr worker) = 0;
};

#endif // RECODEX_BROKER_QUEUE_MANAGER_INTERFACE_HPP
