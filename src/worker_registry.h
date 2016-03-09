#ifndef CODEX_BROKER_ROUTER_H
#define CODEX_BROKER_ROUTER_H

#include <vector>
#include <map>
#include <queue>
#include <memory>

#include "worker.h"

/**
 * Service that stores information about worker machines and routes tasks to them
 */
class worker_registry
{
public:
	typedef std::multimap<std::string, std::string> headers_t;
	typedef std::shared_ptr<worker> worker_ptr;

private:
	std::vector<worker_ptr> workers;

public:
	worker_registry();
	virtual void add_worker(worker_ptr worker);
	virtual void remove_worker(worker_ptr worker);
	virtual worker_ptr find_worker(const headers_t &headers);
	virtual worker_ptr find_worker_by_identity(const std::string &identity);

	/**
	 * Reduce the priority of a worker so that it's less likely to be found by subsequent finds
	 */
	virtual void deprioritize_worker(worker_ptr worker);

	/**
	 * Get all workers known to this service
	 */
	virtual const std::vector<worker_ptr> &get_workers() const;
};

#endif // CODEX_BROKER_ROUTER_H
