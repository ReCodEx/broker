#include "multi_queue_manager.h"

request_ptr multi_queue_manager::add_worker(worker_ptr worker, request_ptr current_request)
{
	queues_.emplace(worker, std::queue<request_ptr>());
	current_requests_.emplace(worker, current_request);
	worker_queue_.push_front(worker);
	return nullptr;
}

std::shared_ptr<std::vector<request_ptr>> multi_queue_manager::worker_terminated(worker_ptr worker)
{
	auto result = std::make_shared<std::vector<request_ptr>>();

	if (current_requests_[worker] != nullptr) {
		result->push_back(current_requests_[worker]);
	}

	while (!queues_[worker].empty()) {
		result->push_back(queues_[worker].front());
		queues_[worker].pop();
	}

	worker_queue_.remove(worker);
	queues_.erase(worker);
	current_requests_.erase(worker);

	return result;
}

enqueue_result multi_queue_manager::enqueue_request(request_ptr request)
{
	enqueue_result result;
	result.enqueued = false;

	// Look for a suitable worker
	worker_ptr worker = nullptr;

	for (auto it = std::begin(worker_queue_); it != std::end(worker_queue_); it++) {
		if ((*it)->check_headers(request->headers)) {
			worker = *it;
			break;
		}
	}

	// If a worker was found, enqueue the request
	if (worker) {
		result.enqueued = true;

		// Move the worker to the end of the queue
		worker_queue_.remove(worker);
		worker_queue_.push_back(worker);

		if (current_requests_[worker] == nullptr) {
			// The worker is free -> assign the request right away
			current_requests_[worker] = request;
			result.assigned_to = worker;
		} else {
			// The worker is occupied -> put the request in its queue
			queues_[worker].push(request);
		}
	}

	return result;
}

request_ptr multi_queue_manager::worker_finished(worker_ptr worker)
{
	current_requests_[worker] = nullptr;

	if (queues_[worker].empty()) {
		return nullptr;
	}

	request_ptr new_request = queues_[worker].front();
	queues_[worker].pop();
	current_requests_[worker] = new_request;

	return new_request;
}

request_ptr multi_queue_manager::get_current_request(worker_ptr worker)
{
	return current_requests_[worker];
}

request_ptr multi_queue_manager::assign_request(worker_ptr worker)
{
	if (queues_[worker].empty()) {
		return nullptr;
	}

	request_ptr new_request = queues_[worker].front();
	queues_[worker].pop();
	current_requests_[worker] = new_request;

	return new_request;
}

request_ptr multi_queue_manager::worker_cancelled(worker_ptr worker)
{
	auto request = current_requests_[worker];
	current_requests_[worker] = nullptr;
	return request;
}

multi_queue_manager::~multi_queue_manager()
{
}

size_t multi_queue_manager::get_queued_request_count()
{
	size_t result = 0;

	for (auto &pair : queues_) {
		result += pair.second.size();
	}

	return result;
}
