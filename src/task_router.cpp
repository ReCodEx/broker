#include "task_router.hpp"


task_router::task_router ()
{
	logger_ = spdlog::get("logger");
}

void task_router::add_worker (worker_ptr worker)
{
	logger_->debug() << "Adding new worker";
	workers.push_back(worker);
}

task_router::worker_ptr task_router::find_worker (const task_router::headers_t &headers)
{
	for (auto &worker: workers) {
		bool worker_suitable = true;

		for (auto &header: headers) {
			bool header_satisfied = false;

			auto range = worker->headers
				.equal_range(header.first);


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
