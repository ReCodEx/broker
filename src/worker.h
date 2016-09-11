#ifndef RECODEX_BROKER_WORKER_H
#define RECODEX_BROKER_WORKER_H

#include <map>
#include <memory>
#include <queue>
#include <vector>


/**
 * Wrapper for request data which holds actual request frames which will be sent to worker
 *   and some more usefull information.
 */
class job_request_data
{
private:
	/** Identification of job. */
	std::string job_id_;
	/** Data frames which will be sent as request to worker. */
	std::vector<std::string> data_;

public:
	/**
	 * Classical constructor with all possible information.
	 * @param job_id identification of job
	 * @param additional additional information which will be added to standard message
	 */
	job_request_data(const std::string &job_id, const std::vector<std::string> &additional)
	{
		job_id_ = job_id;
		data_ = {"eval", job_id};
		for (auto &i : additional) {
			data_.push_back(i);
		}
	}

	/**
	 * Gets job identification of this request.
	 * @return constant reference to string
	 */
	const std::string &get_job_id() const
	{
		return job_id_;
	}

	/**
	 * Gets actual list of frames which will be sent to worker.
	 * @return multipart message
	 */
	std::vector<std::string> get() const
	{
		return data_;
	}
};

/**
 * An evaluation request.
 */
struct request {
	/** Data structure type for holding all headers. */
	typedef std::multimap<std::string, std::string> headers_t;

	/** Headers that specify requirements on the machine that processes the request. */
	const headers_t headers;

	/** The data of the request. */
	const job_request_data data;

	/** The amount of failed attempts at processing this request. */
	size_t failure_count = 0;

	/**
	 * Constructor with initialization.
	 * @param headers Request headers that specify requirements on workers.
	 * @param data Body of the request.
	 */
	request(const headers_t &headers, const job_request_data &data) : headers(headers), data(data)
	{
	}
};

/**
 * Basic matcher used to compare requested headers to those supported by the worker.
 * The class is meant to be extended for different matching methods (e.g. numeric comparison).
 */
class header_matcher
{
protected:
	/** Value for comparing with headers. */
	std::string my_value_;

public:
	/**
	 * Constructor setting base value to be compared to.
	 * @param my_value Base value.
	 */
	header_matcher(std::string my_value) : my_value_(my_value)
	{
	}

	/** Destructor. */
	virtual ~header_matcher()
	{
	}

	/**
	 * Check if given value match with inner preset value.
	 * @param value Value to be checked.
	 * @return @a true if value matches, @a false otherwise.
	 */
	virtual bool match(const std::string &value)
	{
		return value == my_value_;
	}
};

/**
 * Contains information about a worker machine.
 */
class worker
{
public:
	/** Pointer to evauate request type. */
	typedef std::shared_ptr<request> request_ptr;

private:
	/** Headers that describe the workers capabilities. */
	std::multimap<std::string, std::unique_ptr<header_matcher>> headers_;

	/** @a false if the worker is processing a request. */
	bool free_;

	/** A queue of requests to be processed by the worker. */
	std::queue<request_ptr> request_queue_;

	/** The request that is now being processed by the worker. */
	request_ptr current_request_;

public:
	/** A unique identifier of the worker. */
	const std::string identity;

	/** A hardware group identifier. */
	const std::string hwgroup;

	/** The amount of pings the worker can miss before it's considered dead. */
	size_t liveness;

	/**
	 * Constructor.
	 * @param id Worker unique identifier.
	 * @param hwgroup Worker handrware group identifier.
	 * @param headers Headers describing worker capabilities.
	 */
	worker(const std::string &id, const std::string &hwgroup, const std::multimap<std::string, std::string> &headers);

	/**
	 * Stated for completion.
	 */
	virtual ~worker();

	/**
	 * Check if the worker satisfies given header.
	 * @param header Name of the header.
	 * @param value Value to be checked against the header.
	 */
	virtual bool check_header(const std::string &header, const std::string &value);

	/**
	 * Insert a request into the workers queue.
	 * @param request A pointer to the request.
	 */
	virtual void enqueue_request(request_ptr request);

	/**
	 * Consider the current request complete.
	 * Called when the actual worker machine successfully processes the request.
	 */
	virtual void complete_request();

	/**
	 * If possible, take a request from the queue and start processing it.
	 * @return @a true if and only if the worker started processing a new request.
	 */
	virtual bool next_request();

	/**
	 * Get the request that is now being processed.
	 * @return Pointer to currently processed request or @a nullptr.
	 */
	virtual std::shared_ptr<const request> get_current_request() const;

	/**
	 * Forget all requests (currently processed and queued).
	 * Called when the worker is considered dead.
	 * @return Current request and all requests from waiting queue.
	 */
	virtual std::shared_ptr<std::vector<request_ptr>> terminate();
};

#endif // RECODEX_BROKER_WORKER_H
