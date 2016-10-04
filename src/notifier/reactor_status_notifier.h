#ifndef RECODEX_BROKER_REACTOR_STATUS_NOTIFIER_H
#define RECODEX_BROKER_REACTOR_STATUS_NOTIFIER_H

#include "../config/notifier_config.h"
#include "../helpers/curl.h"
#include "../reactor/handler_interface.h"
#include "status_notifier.h"
#include <spdlog/logger.h>

/**
 * A status notifier that forwards status messages to a handler registered in our ZeroMQ reactor.
 * This enables us to send the messages asynchronously - the broker doesn't have to wait for HTTP requests
 */
class reactor_status_notifier : public status_notifier_interface
{
private:
	/** A callback for sending messages through the reactor */
	handler_interface::response_cb callback_;

	/** The name under which the status notifier handler is registered in the reactor*/
	const std::string key_;

public:
	/**
	 * @param callback A callback that sends messages through a reactor. It is expected this class will be used
	 *                 in reactor event handlers, hence the callback type.
	 * @param key Reactor event key for messages sent by the notifier
	 */
	reactor_status_notifier(handler_interface::response_cb callback, const std::string &key);

	virtual void error(const std::string &desc);
	virtual void rejected_job(const std::string &job_id, const std::string &desc = "");
	virtual void rejected_jobs(std::vector<std::string> job_ids, const std::string &desc = "");
	virtual void job_done(const std::string &job_id);
	virtual void job_failed(const std::string &job_id, const std::string &desc = "");
};

#endif // RECODEX_BROKER_REACTOR_STATUS_NOTIFIER_H
