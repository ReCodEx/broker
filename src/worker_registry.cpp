#include "worker_registry.h"

#include <algorithm>


worker_registry::worker_registry()
{
}

void worker_registry::add_worker(worker_ptr worker)
{
	workers.push_back(worker);
}

void worker_registry::remove_worker(worker_ptr worker)
{
	auto it = std::find(std::begin(workers), std::end(workers), worker);

	if (it != std::end(workers)) {
		workers.erase(it);
	}
}

worker_registry::worker_ptr worker_registry::find_worker(const request::headers_t &headers)
{
	for (auto &worker : workers) {
		bool is_worker_suitable = true;

		for (auto &header : headers) {
			if (!worker->check_header(header.first, header.second)) {
				is_worker_suitable = false;
				break;
			}
		}

		if (is_worker_suitable) {
			return worker;
		}
	}

	return nullptr;
}

worker_registry::worker_ptr worker_registry::find_worker_by_identity(const std::string &identity)
{
	for (auto &worker : workers) {
		if (worker->identity == identity) {
			return worker;
		}
	}

	return nullptr;
}

void worker_registry::deprioritize_worker(worker_registry::worker_ptr worker)
{
	auto it = std::find(std::begin(workers), std::end(workers), worker);

	if (it != std::end(workers) && (it + 1) != std::end(workers)) {
		workers.erase(it);
		workers.push_back(worker);
	}
}

const std::vector<worker_registry::worker_ptr> &worker_registry::get_workers() const
{
	return workers;
}
