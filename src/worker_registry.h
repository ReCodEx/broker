#ifndef RECODEX_BROKER_ROUTER_H
#define RECODEX_BROKER_ROUTER_H

#include <map>
#include <memory>
#include <queue>
#include <vector>

#include "worker.h"

/**
 * Service that stores information about worker machines and routes tasks to them.
 */
class worker_registry
{
public:
	/** Pointer to worker instance type. */
	typedef std::shared_ptr<worker> worker_ptr;

private:
	/** List of known workers. */
	std::vector<worker_ptr> workers_;

public:
	/** Default constructor, initializes empty list of workers. */
	worker_registry();
	/**
	 * Adds new worker to the registry.
	 * @param worker Worker instance to be added.
	 */
	virtual void add_worker(worker_ptr worker);
	/**
	 * Remove worker from registry, for example when connection is lost.
	 * @param worker Worker instance to be removed.
	 */
	virtual void remove_worker(worker_ptr worker);
	/**
	 * Find suitable worker on the basis of request headers.
	 * @param headers Reqeust headers which we want to be fulfiled by any worker.
	 * @return Instance to first suitable worker found or @a nullptr.
	 */
	virtual worker_ptr find_worker(const request::headers_t &headers);
	/**
	 * Find worker by it's unique identifier.
	 * @param identity ID of worker we want to get instance.
	 * @return Instance of worker with given ID or @a nullptr.
	 */
	virtual worker_ptr find_worker_by_identity(const std::string &identity);
	/**
	 * Get all workers known to this service.
	 * @return Collection of all known workers.
	 */
	virtual const std::vector<worker_ptr> &get_workers() const;
};

#endif // RECODEX_BROKER_ROUTER_H
