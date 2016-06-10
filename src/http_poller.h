#ifndef CODEX_HTTP_POLLER_H
#define CODEX_HTTP_POLLER_H

#include <spdlog/spdlog.h>
#include "helpers/curl.h"
#include "helpers/logger.h"
#include "config/frontend_config.h"


/**
 * HTTP poller is used for connection between broker and frontend.
 * Broker can communicate with frontend through HTTP requests.
 */
class http_poller
{
private:
	/** Configuration of http_poller */
	frontend_config config_;
	/** Textual representation of url with port included */
	std::string address_;

	/** Logger class */
	std::shared_ptr<spdlog::logger> logger_;

public:
	/**
	 * Config has to be handed over to http_poller on construction.
	 * @param config configuration of http_poller
	 * @param logger standard way of logging through spdlog
	 */
	http_poller(const frontend_config &config, std::shared_ptr<spdlog::logger> logger = nullptr);

	/**
	 * Send job_done message to frontend.
	 * @param job_id identification of executed job
	 */
	void send_job_done(std::string job_id);

	/**
	 * Basically tells frontend that there was some serious problem which has to be solved by administrator.
	 * @param desc description of error which was caused in broker
	 */
	void send_error(std::string desc);
};

#endif // CODEX_HTTP_POLLER_H
