#include "worker.h"

/**
 * Takes multiple string values separated by the "|" character and checks if the workers value is equal at least to
 * one of these.
 */
class multiple_string_matcher : public header_matcher {
public:
	static const char delimiter = '|';

	multiple_string_matcher(std::string my_value) : header_matcher(my_value)
	{
	}

	virtual bool match(const std::string &value)
	{
		size_t offset = 0;

		while (offset < value.size()) {
			size_t end = value.find(delimiter, offset);

			if (end == std::string::npos) {
				end = value.size();
			}

			if (value.compare(offset, end - offset, my_value_) == 0) {
				return true;
			}

			offset = end + 1;
		}

		return false;
	}
};

/**
 * Checks if a value is less than or equal to a set number
 */
class count_matcher : public header_matcher {
private:
	size_t my_count_;
public:
	count_matcher(std::string my_value) : my_count_(std::stoul(my_value)), header_matcher(my_value)
	{
	}

	virtual bool match(const std::string &value)
	{
		return my_count_ >= std::stoul(value);
	}
};

worker::worker(const std::string &id, const std::string &hwgroup, const std::multimap<std::string, std::string> &headers)
	: identity(id), hwgroup(hwgroup), free(true), current_request(nullptr)
{
	headers_.emplace("hwgroup", std::unique_ptr<header_matcher>(new multiple_string_matcher(hwgroup)));

	for (auto it: headers) {
		auto matcher = std::unique_ptr<header_matcher>(new header_matcher(it.second));

		if (it.first == "threads") {
			matcher = std::unique_ptr<header_matcher>(new count_matcher(it.second));
		}

		headers_.emplace(it.first, std::move(matcher));
	}
}

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

	if (current_request != nullptr) {
		current_request->failure_count += 1;
		result->push_back(current_request);
	}

	current_request = nullptr;

	while (!request_queue.empty()) {
		result->push_back(request_queue.front());
		request_queue.pop();
	}

	return result;
}

bool worker::check_header(const std::string &header, const std::string &value)
{
	// Find all worker headers with the right name
	auto range = headers_.equal_range(header);
	for (auto &worker_header = range.first; worker_header != range.second; ++worker_header) {
		// If any of these headers has a matching value, return true
		if (worker_header->second->match(value)) {
			return true;
		}
	}

	// If we found nothing, return false
	return false;
}
