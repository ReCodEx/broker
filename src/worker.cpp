#include "worker.h"

void worker::enqueue_request(request_ptr request)
{
	request_queue.push(request);
}

void worker::complete_request()
{
	free = true;
	current_request = nullptr;
}

bool worker::next_request()
{
	if (free && !request_queue.empty()) {
		current_request = request_queue.front();
		request_queue.pop();
		free = false;

		return true;
	}

	return false;
}

std::shared_ptr<const request> worker::get_current_request() const
{
	return current_request;
}

std::shared_ptr<std::vector<worker::request_ptr>> worker::terminate()
{
	auto result = std::make_shared<std::vector<worker::request_ptr>>();

	current_request->failure_count += 1;
	result->push_back(current_request);
	current_request = nullptr;

	while (!request_queue.empty()) {
		result->push_back(request_queue.front());
		request_queue.pop();
	}

	return result;
}
