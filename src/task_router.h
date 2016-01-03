#ifndef CODEX_BROKER_ROUTER_H
#define CODEX_BROKER_ROUTER_H

#include <vector>
#include <map>
#include <memory>
#include <spdlog/spdlog.h>

/* Forward declaration */
struct worker;

/**
 * Service that stores information about worker machines and routes tasks to them
 */
class task_router {
public:
	typedef std::multimap<std::string, std::string> headers_t;
	typedef std::shared_ptr<worker> worker_ptr;
private:
	std::vector<worker_ptr> workers;
	std::shared_ptr<spdlog::logger> logger_;
public:
	task_router (std::shared_ptr<spdlog::logger> logger = nullptr);
	virtual void add_worker (worker_ptr worker);
	virtual worker_ptr find_worker (const headers_t &headers);
};

/**
 * A structure that contains data about a worker machine
 */
struct worker {
	/** A unique identifier of the worker */
	const std::string identity;

	/** Headers that describe the worker's capabilities */
	const task_router::headers_t headers;

	worker (std::string id, task_router::headers_t headers): identity(id), headers(headers)
	{
	}
};

#endif //CODEX_BROKER_ROUTER_H
