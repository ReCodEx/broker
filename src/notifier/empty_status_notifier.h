#ifndef RECODEX_EMPTY_STATUS_NOTIFIER_H
#define RECODEX_EMPTY_STATUS_NOTIFIER_H

#include "status_notifier.h"


/**
 * A status notifier that does nothing when an error occurs.
 */
class empty_status_notifier : public status_notifier_interface
{
public:
	virtual void error(std::string desc)
	{
	}

	virtual void rejected_job(std::string job_id, std::string desc = "")
	{
	}

	virtual void rejected_jobs(std::vector<std::string> job_ids, std::string desc = "")
	{
	}

	virtual void job_failed(std::string job_id, std::string desc = "")
	{
	}
};

#endif // RECODEX_EMPTY_STATUS_NOTIFIER_H
