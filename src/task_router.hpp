#ifndef CODEX_BROKER_ROUTER_H
#define CODEX_BROKER_ROUTER_H

#include <vector>
#include <map>
#include <memory>
#include "spdlog/spdlog.h"

/* Forward declaration */
struct worker;

class task_router {
public:
	typedef std::multimap<std::string, std::string> headers_t;
	typedef std::shared_ptr<worker> worker_ptr;
private:
	std::vector<worker_ptr> workers;
	std::shared_ptr<spdlog::logger> logger_;
public:
	task_router ();
	void add_worker (worker_ptr worker);
};

struct worker {
	const std::string &identity;
	const task_router::headers_t &headers;

	worker (std::string id, task_router::headers_t headers): identity(id), headers(headers)
	{
	}
};

#endif //CODEX_BROKER_ROUTER_H
