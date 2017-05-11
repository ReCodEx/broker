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
	/** Indicates whether the object contains the request frames */
	bool complete_;

public:
	/**
	 * Default constructor
	 * @param job_id identification of job
	 * @param additional additional information which will be added to standard message
	 */
	job_request_data(const std::string &job_id, const std::vector<std::string> &additional) : complete_(true)
	{
		job_id_ = job_id;
		data_ = {"eval", job_id};
		for (auto &i : additional) {
			data_.push_back(i);
		}
	}

	/**
	 * A constructor for incomplete jobs (only id without request frames)
	 * @param job_id identification of job
	 */
	job_request_data(const std::string &job_id) : job_id_(job_id), complete_(false)
	{
	}

	/**
	 * Is the request data complete?
	 * @return true if and only if the request data is complete
	 */
	bool is_complete() const
	{
		return complete_;
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

	/**
	 * A constructor for incomplete requests
	 * @param data Body of the request.
	 */
	request(const job_request_data &data) : data(data)
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

	/** A copy of the headers used to instantiate the worker (used by comparison) */
	const std::multimap<std::string, std::string> headers_copy_;

public:
	/** A unique identifier of the worker. */
	const std::string identity;

	/** An optional human readable description */
	std::string description = "";

	/** A hardware group identifier. */
	const std::string hwgroup;

	/** The amount of pings the worker can miss before it's considered dead. */
	size_t liveness;

	/**
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
	 * Check if the worker's headers are equal to given set of headers
	 * @param other The set of headers to check
	 */
	bool headers_equal(const std::multimap<std::string, std::string> &other);

	/**
	 * Check if the worker satisfies given header.
	 * @param header Name of the header.
	 * @param value Value to be checked against the header.
	 */
	virtual bool check_header(const std::string &header, const std::string &value);

	/**
	 * Check if the worker satisfies given header set.
	 * @param headers A key-value set of headers
	 */
	virtual bool check_headers(const std::multimap<std::string, std::string> &headers);

	/**
	 * Get a textual description of the worker
	 * @return textual description of the worker
	 */
	std::string get_description() const;
};

#endif // RECODEX_BROKER_WORKER_H
