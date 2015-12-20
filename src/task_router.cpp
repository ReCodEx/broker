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
