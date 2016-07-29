#ifndef RECODEX_EMPTY_STATUS_NOTIFIER_H
#define RECODEX_EMPTY_STATUS_NOTIFIER_H

#include "status_notifier.h"


/**
 * A status notifier that does nothing when an error occurs.
 */
class empty_status_notifier : public status_notifier
{
public:
	/**
	 * Empty implementation.
	 * @param desc description of error which was caused in broker
	 */
	virtual void send_error(std::string desc)
	{
	}
};

#endif // RECODEX_EMPTY_STATUS_NOTIFIER_H
