#include "worker_registry.h"

#include <algorithm>


worker_registry::worker_registry()
{
}

void worker_registry::add_worker(worker_ptr worker)
{
	workers_.push_back(worker);
}

void worker_registry::remove_worker(worker_ptr worker)
{
	auto it = std::find(std::begin(workers_), std::end(workers_), worker);

	if (it != std::end(workers_)) {
		workers_.erase(it);
	}
}

worker_registry::worker_ptr worker_registry::find_worker(const request::headers_t &headers)
{
	for (auto &worker : workers_) {
		if (worker->check_headers(headers)) {
			return worker;
		}
	}

	return nullptr;
}

worker_registry::worker_ptr worker_registry::find_worker_by_identity(const std::string &identity)
{
	for (auto &worker : workers_) {
		if (worker->identity == identity) {
			return worker;
		}
	}

	return nullptr;
}

const std::vector<worker_registry::worker_ptr> &worker_registry::get_workers() const
{
	return workers_;
}
