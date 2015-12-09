#ifndef CODEX_BROKER_ROUTER_H
#define CODEX_BROKER_ROUTER_H

#include <vector>
#include <map>

/* Forward declaration */
struct worker;

class task_router {
public:
	typedef std::multimap<std::string, std::string> headers_t;
private:
	std::vector<worker> workers;

public:
	void add_worker (worker worker);

};

struct worker {
	worker ();
	task_router::headers_t headers;
};

#endif //CODEX_BROKER_ROUTER_H
