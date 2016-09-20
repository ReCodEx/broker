#include "worker.h"
#include "helpers/string_to_hex.h"

/**
 * Takes string containing multiple values (separated by "|" character) and checks, if any of them is equal to
 * preset value.
 */
class multiple_string_matcher : public header_matcher
{
public:
	/** String value delimiter. */
	static const char delimiter = '|';

	/**
	 * Constructor setting base value to be compared to.
	 * @param my_value Base value.
	 */
	multiple_string_matcher(std::string my_value) : header_matcher(my_value)
	{
	}

	/** Destructor. */
	virtual ~multiple_string_matcher()
	{
	}

	/**
	 * Check if any of given values match with inner preset value.
	 * @param value Value to be checked. It's one string containing multiple values delimited by @ref delimiter.
	 * @return @a true if any of the values matches, @a false otherwise.
	 */
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
 * Checks if a given value is less than or equal to a preset number.
 */
class count_matcher : public header_matcher
{
private:
	/** Preset number to be base for comparison. */
	size_t my_count_;

public:
	/**
	 * Constructor.
	 * @param my_value Base value for comparison. Will be converted to @a size_t type using std::stoul.
	 */
	count_matcher(std::string my_value) : my_count_(std::stoul(my_value)), header_matcher(my_value)
	{
	}

	/** Destructor. */
	virtual ~count_matcher()
	{
	}

	/**
	 * Check if given value is >= to inner preset value.
	 * @param value Value to be checked. Before comparison, the value is converted to @a size_t type using std::stoul.
	 * @return @a true if value matches, @a false otherwise.
	 */
	virtual bool match(const std::string &value)
	{
		return my_count_ >= std::stoul(value);
	}
};


worker::worker(
	const std::string &id, const std::string &hwgroup, const std::multimap<std::string, std::string> &headers)
	: identity(id), hwgroup(hwgroup), free_(true), current_request_(nullptr), headers_copy_(headers)
{
	headers_.emplace("hwgroup", std::unique_ptr<header_matcher>(new multiple_string_matcher(hwgroup)));

	for (auto it : headers) {
		auto matcher = std::unique_ptr<header_matcher>(new header_matcher(it.second));

		if (it.first == "threads") {
			matcher = std::unique_ptr<header_matcher>(new count_matcher(it.second));
		}

		headers_.emplace(it.first, std::move(matcher));
	}
}

worker::~worker()
{
}

void worker::enqueue_request(request_ptr request)
{
	request_queue_.push(request);
}

void worker::complete_request()
{
	free_ = true;
	current_request_ = nullptr;
}

bool worker::next_request()
{
	if (free_ && !request_queue_.empty()) {
		current_request_ = request_queue_.front();
		request_queue_.pop();
		free_ = false;

		return true;
	}

	return false;
}

std::shared_ptr<const request> worker::get_current_request() const
{
	return current_request_;
}

std::shared_ptr<std::vector<worker::request_ptr>> worker::terminate()
{
	auto result = std::make_shared<std::vector<worker::request_ptr>>();

	if (current_request_ != nullptr) {
		current_request_->failure_count += 1;
		result->push_back(current_request_);
	}

	current_request_ = nullptr;

	while (!request_queue_.empty()) {
		result->push_back(request_queue_.front());
		request_queue_.pop();
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


bool worker::headers_equal(const std::multimap<std::string, std::string> &other) {
	return other == headers_copy_;
}

std::string worker::get_description () const
{
	return helpers::string_to_hex(identity);
}
