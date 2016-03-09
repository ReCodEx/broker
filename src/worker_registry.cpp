#include "worker_registry.h"

#include <algorithm>

worker_registry::worker_registry(std::shared_ptr<spdlog::logger> logger)
{
	// TODO: logger currently not used here. Can be removed?
	if (logger != nullptr) {
		logger_ = logger;
	} else {
		// Create logger manually to avoid global registration of logger
		auto sink = std::make_shared<spdlog::sinks::stderr_sink_st>();
		logger_ = std::make_shared<spdlog::logger>("task_router_nolog", sink);
		// Set loglevel to 'off' cause no logging
		logger_->set_level(spdlog::level::off);
	}
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

worker_registry::worker_ptr worker_registry::find_worker(const worker_registry::headers_t &headers)
{
	for (auto &worker : workers) {
		bool worker_suitable = true;

		for (auto &header : headers) {
			bool header_satisfied = false;

			auto range = worker->headers.equal_range(header.first);

			for (auto &worker_header = range.first; worker_header != range.second; ++worker_header) {
				if (worker_header->second == header.second) {
					header_satisfied = true;
					break;
				}
			}

			if (!header_satisfied) {
				worker_suitable = false;
				break;
			}
		}

		if (worker_suitable) {
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