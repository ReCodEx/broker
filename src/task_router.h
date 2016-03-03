#ifndef CODEX_BROKER_ROUTER_H
#define CODEX_BROKER_ROUTER_H

#include <vector>
#include <map>
#include <queue>
#include <memory>
#include <spdlog/spdlog.h>

/* Forward declaration */
struct worker;

/**
 * Service that stores information about worker machines and routes tasks to them
 */
class task_router
{
public:
	typedef std::multimap<std::string, std::string> headers_t;
	typedef std::shared_ptr<worker> worker_ptr;
	typedef std::vector<std::string> request_t;

private:
	std::vector<worker_ptr> workers;
	std::shared_ptr<spdlog::logger> logger_;

public:
	task_router(std::shared_ptr<spdlog::logger> logger = nullptr);
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

/**
 * A structure that contains data about a worker machine
 */
struct worker {
	/** A unique identifier of the worker */
	const std::string identity;

	/** Headers that describe the worker's capabilities */
	const task_router::headers_t headers;

	/** False if the worker is processing a request */
	bool free;

	/** A queue of requests to be processed by the worker */
	std::queue<task_router::request_t> request_queue;

	/** The amount of pings the worker can miss before it's considered dead */
	size_t liveness;

	worker(const std::string &id, const task_router::headers_t &headers) : identity(id), headers(headers), free(true)
	{
	}
};

#endif // CODEX_BROKER_ROUTER_H
