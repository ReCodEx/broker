#ifndef RECODEX_HTTP_STATUS_NOTIFIER_H
#define RECODEX_HTTP_STATUS_NOTIFIER_H

#include "../config/frontend_config.h"
#include "../helpers/curl.h"
#include "../helpers/logger.h"
#include "status_notifier.h"
#include <spdlog/spdlog.h>


/**
 * A status notifier that uses HTTP (specifically the REST API) to notify the frontend of errors.
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
	 * @param config configuration of frontend connection
	 * @param logger a logger object used when errors happen @ref send_error
	 */
	http_status_notifier(const frontend_config &config, std::shared_ptr<spdlog::logger> logger = nullptr);

	virtual void send_error(std::string desc);
};

#endif // RECODEX_HTTP_STATUS_NOTIFIER_H
