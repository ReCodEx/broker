#ifndef RECODEX_EMPTY_STATUS_NOTIFIER_H
#define RECODEX_EMPTY_STATUS_NOTIFIER_H

#include "status_notifier.h"


/**
 * A status notifier that does nothing when an error occurs.
 */
class empty_status_notifier : public status_notifier_interface
{
public:
	void error(const std::string &desc) override
	{
	}

	void rejected_job(const std::string &job_id, const std::string &desc = "") override
	{
	}

	void rejected_jobs(std::vector<std::string> job_ids, const std::string &desc = "") override
	{
	}

	void job_done(const std::string &job_id) override
	{
	}

	void job_failed(const std::string &job_id, const std::string &desc = "") override
	{
	}
};

#endif // RECODEX_EMPTY_STATUS_NOTIFIER_H
