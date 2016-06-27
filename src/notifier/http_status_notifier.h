#ifndef RECODEX_HTTP_STATUS_NOTIFIER_H
#define RECODEX_HTTP_STATUS_NOTIFIER_H

#include "../config/frontend_config.h"
#include "../helpers/curl.h"
#include "../helpers/logger.h"
#include "status_notifier.h"
#include <spdlog/spdlog.h>


/**
 * HTTP status notifier is used for connection between broker and frontend.
 * Broker can communicate with frontend through HTTP requests.
 */
class http_status_notifier : public status_notifier
{
private:
	/** Configuration of frontend address and connection */
	frontend_config config_;
	/** Textual representation of url with port included */
	std::string address_;

	/** Logger class */
	std::shared_ptr<spdlog::logger> logger_;

public:
	/**
	 * Stated for completion.
	 */
	virtual ~http_status_notifier();

	/**
	 * Config has to be handed over to notifier on construction.
	 * @param config configuration of frontend connection
	 * @param logger standard way of logging through spdlog
	 */
	http_status_notifier(const frontend_config &config, std::shared_ptr<spdlog::logger> logger = nullptr);

	/**
	 * Basically tells frontend that there was some serious problem which has to be solved by administrator.
	 * @param desc description of error which was caused in broker
	 */
	virtual void send_error(std::string desc);
};

#endif // RECODEX_HTTP_STATUS_NOTIFIER_H
