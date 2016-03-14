#ifndef RECODEX_BROKER_WORKER_H
#define RECODEX_BROKER_WORKER_H

#include <memory>
#include <vector>
#include <queue>
#include <map>

/**
 * An evaluation request
 */
struct request {
	typedef std::multimap<std::string, std::string> headers_t;

	/** Headers that specify requirements on the machine that processes the request */
	const headers_t headers;

	/** The data of the request */
	const std::vector<std::string> data;

	/** The amount of failed attempts at processing this request */
	size_t failure_count = 0;

	request(const headers_t &headers, const std::vector<std::string> &data) : headers(headers), data(data)
	{
	}
};

/**
 * Used to compare requested headers to those supported by the worker.
 * The class is meant to be extended for different matching methods (e.g. numeric comparison)
 */
class header_matcher
{
protected:
	std::string my_value_;

public:
	header_matcher(std::string my_value) : my_value_(my_value)
	{
	}

	virtual bool match(const std::string &value)
	{
		return value == my_value_;
	}
};

/**
 * Contains information about a worker machine
 */
class worker
{
public:
	typedef std::shared_ptr<request> request_ptr;

private:
	/** Headers that describe the workers capabilities */
	std::multimap<std::string, std::unique_ptr<header_matcher>> headers_;

	/** False if the worker is processing a request */
	bool free;

	/** A queue of requests to be processed by the worker */
	std::queue<request_ptr> request_queue;

	/** The request that is now being processed by the worker */
	request_ptr current_request;

public:
	/** A unique identifier of the worker */
	const std::string identity;

	/** A hardware group identifier */
	const std::string hwgroup;

	/** The amount of pings the worker can miss before it's considered dead */
	size_t liveness;

	worker(const std::string &id, const std::string &hwgroup, const std::multimap<std::string, std::string> &headers);

	/**
	 * Check if the worker satisfies given header
	 * @param header Name of the header
	 * @param value Name of the value
	 */
	bool check_header(const std::string &header, const std::string &value);

	/**
	 * Insert a request into the workers queue
	 * @param request A pointer to the request
	 */
	void enqueue_request(request_ptr request);

	/**
	 * Consider the current request complete.
	 * Called when the actual worker machine successfully processes the request
	 */
	void complete_request();

	/**
	 * If possible, take a request from the queue and start processing it
	 * @return true if and only if the worker started processing a new request
	 */
	bool next_request();

	/**
	 * Get the request that is now being processed
	 */
	std::shared_ptr<const request> get_current_request() const;

	/**
	 * Forget all requests (currently processed and queued).
	 * Called when the worker is considered dead.
	 */
	std::shared_ptr<std::vector<request_ptr>> terminate();
};

#endif // RECODEX_BROKER_WORKER_H
