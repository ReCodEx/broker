#include "task_router.h"


task_router::task_router(std::shared_ptr<spdlog::logger> logger)
{
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

void task_router::add_worker(worker_ptr worker)
{
	logger_->debug() << "Adding new worker";
	workers.push_back(worker);
}

task_router::worker_ptr task_router::find_worker(const task_router::headers_t &headers)
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

task_router::worker_ptr task_router::find_worker_by_identity (const std::string &identity)
{
	for (auto &worker: workers) {
		if (worker->identity == identity) {
			return worker;
		}
	}

	return nullptr;
}
