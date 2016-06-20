#ifndef CODEX_EMPTY_STATUS_NOTIFIER_H
#define CODEX_EMPTY_STATUS_NOTIFIER_H

#include "status_notifier.h"


/**
 * This status notifier has all methods implementation empty.
 */
class empty_status_notifier : public status_notifier
{
public:
	/**
	 * Stated for completion.
	 */
	virtual ~empty_status_notifier()
	{
	}

	/**
	 * Empty implementation.
	 * @param job_id identification of executed job
	 */
	virtual void send_job_done(std::string job_id)
	{
	}

	/**
	 * Empty implementation.
	 * @param desc description of error which was caused in broker
	 */
	virtual void send_error(std::string desc)
	{
	}
};

#endif // CODEX_EMPTY_STATUS_NOTIFIER_H
