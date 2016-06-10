#ifndef CODEX_STATUS_NOTIFIER_H
#define CODEX_STATUS_NOTIFIER_H


/**
 * Status notifier defines interface which can be used to report errors
 *   or some other states of broker.
 */
class status_notifier
{
public:
	/**
	 * Stated for completion.
	 */
	virtual ~status_notifier()
	{
	}

	/**
	 * Send job_done message.
	 * @param job_id identification of executed job
	 */
	virtual void send_job_done(std::string job_id) = 0;

	/**
	 * Basically tells that there was some serious problem which cannot be solved.
	 * @param desc description of error which was caused in broker
	 */
	virtual void send_error(std::string desc) = 0;
};

#endif // CODEX_STATUS_NOTIFIER_H
