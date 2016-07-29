#ifndef RECODEX_STATUS_NOTIFIER_H
#define RECODEX_STATUS_NOTIFIER_H


/**
 * Status notifier defines interface which can be used to report errors
 *   or some other states of broker.
 * All methods should be exceptionless and logging mechanism is advised to used on errors.
 */
class status_notifier
{
public:
	/**
	 * Basically tells frontend that there was some serious problem which has to be solved by administrator.
	 * @param desc description of error which was caused in broker
	 */
	virtual void send_error(std::string desc) = 0;
};

#endif // RECODEX_STATUS_NOTIFIER_H
