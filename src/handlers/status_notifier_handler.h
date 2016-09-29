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
	static const std::string TYPE_ERROR;
	static const std::string TYPE_JOB_STATUS;

	status_notifier_handler(const notifier_config &config, std::shared_ptr<spdlog::logger> logger);
	void on_request(const message_container &message, response_cb respond);

private:
	const notifier_config config_;
	std::shared_ptr<spdlog::logger> logger_;
};

#endif // RECODEX_BROKER_STATUS_NOTIFIER_HANDLER_H
