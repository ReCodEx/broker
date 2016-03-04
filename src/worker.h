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
	/** Headers that specify requirements on the machine that processes the request */
	const std::multimap<std::string, std::string> headers;

	/** The data of the request */
	const std::vector<std::string> data;

	/** The amount of failed attempts at processing this request */
	size_t failure_count = 0;

	request(const std::multimap<std::string, std::string> &headers, const std::vector<std::string> &data)
		: headers(headers), data(data)
	{
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
	/** False if the worker is processing a request */
	bool free;

	/** A queue of requests to be processed by the worker */
	std::queue<request_ptr> request_queue;

	/** The request that is now being processed by the worker */
	request_ptr current_request;

public:
	/** A unique identifier of the worker */
	const std::string identity;

	/** Headers that describe the worker's capabilities */
	const std::multimap<std::string, std::string> headers;

	/** The amount of pings the worker can miss before it's considered dead */
	size_t liveness;

	worker(const std::string &id, const std::multimap<std::string, std::string> &headers)
		: identity(id), headers(headers), free(true), current_request(nullptr)
	{
	}

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
