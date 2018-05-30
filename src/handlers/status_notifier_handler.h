#ifndef RECODEX_BROKER_STATUS_NOTIFIER_HANDLER_H
#define RECODEX_BROKER_STATUS_NOTIFIER_HANDLER_H

#include <memory>
#include <spdlog/logger.h>

#include "../config/notifier_config.h"
#include "../notifier/status_notifier.h"
#include "../reactor/handler_interface.h"

/**
 * Receives status notifications from the reactor and forwards them to the frontend REST API
 */
class status_notifier_handler : public handler_interface
{
public:
	/**
	 * REST API endpoint that receives error messages
	 */
	static const std::string TYPE_ERROR;

	/**
	 * REST API endpoint that receives job status messages
	 */
	static const std::string TYPE_JOB_STATUS;

	/**
	 * @param config A notifier configuration (contains URL and credentials for the REST API)
	 * @param logger A logger used when a HTTP request fails
	 */
	status_notifier_handler(const notifier_config &config, std::shared_ptr<spdlog::logger> logger);

	/** Destructor */
	~status_notifier_handler() override = default;

	void on_request(const message_container &message, const response_cb &respond) override;

private:
	/**
	 * A notifier configuration
	 */
	const notifier_config config_;

	/**
	 * The system logger
	 */
	std::shared_ptr<spdlog::logger> logger_;
};

#endif // RECODEX_BROKER_STATUS_NOTIFIER_HANDLER_H
