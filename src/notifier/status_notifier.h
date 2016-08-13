#ifndef RECODEX_STATUS_NOTIFIER_H
#define RECODEX_STATUS_NOTIFIER_H

#include <vector>
#include <string>


/**
 * Status notifier defines interface which can be used to report errors
 *   or some other states of broker.
 * @note All methods should be exceptionless and logging mechanism is advised to used on errors.
 */
class status_notifier_interface
{
public:
	/**
	 * Basically tells frontend that there was some serious problem which has to be solved by administrator.
	 * Should be used only for generic errors which does not need any special treatment.
	 * @param desc description of error which was caused in broker
	 * @note Implementation should be exceptionless.
	 */
	virtual void error(std::string desc) = 0;

	/**
	 * Notify frontend that broker had to throw away some jobs and they have to be execute again.
	 * This might happen for instance if worker dies and there is none with the same headers.
	 * @param job_ids list of job ids which was not executed
	 * @note Implementation should be exceptionless.
	 */
	virtual void rejected_jobs(std::vector<std::string> job_ids) = 0;

	/**
	 * Is called when worker return job results with status not equal to OK.
	 * @param job_id identification of failed job
	 * @note Implementation should be exceptionless.
	 */
	virtual void job_failed(std::string job_id) = 0;
};

#endif // RECODEX_STATUS_NOTIFIER_H
