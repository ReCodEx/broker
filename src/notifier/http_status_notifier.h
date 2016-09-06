#ifndef RECODEX_HTTP_STATUS_NOTIFIER_H
#define RECODEX_HTTP_STATUS_NOTIFIER_H

#include "../config/notifier_config.h"
#include "../helpers/curl.h"
#include "../helpers/logger.h"
#include "status_notifier.h"
#include <spdlog/spdlog.h>


/**
 * A status notifier that uses HTTP (specifically the REST API) to notify the frontend of errors.
 */
class http_status_notifier : public status_notifier_interface
{
private:
	/** Configuration of frontend address and connection */
	notifier_config config_;
	/** Textual representation of url with port included */
	std::string address_;

	/** Logger class */
	std::shared_ptr<spdlog::logger> logger_;

	/**
	 * Internal helper function which is used for actual sending of HTTP requests.
	 * @param route defines route to which base address will be added
	 * @param params parameters which will be added to request
	 */
	void send(std::string route, helpers::curl_params params);

public:
	/**
	 * @param config configuration of frontend connection
	 * @param logger a logger object used when errors happen
	 */
	http_status_notifier(const notifier_config &config, std::shared_ptr<spdlog::logger> logger = nullptr);

	virtual void error(const std::string &desc);
	virtual void rejected_job(const std::string &job_id, const std::string &desc = "");
	virtual void rejected_jobs(std::vector<std::string> job_ids, const std::string &desc = "");
	virtual void job_done(const std::string &job_id);
	virtual void job_failed(const std::string &job_id, const std::string &desc = "");
};

#endif // RECODEX_HTTP_STATUS_NOTIFIER_H
