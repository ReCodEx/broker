#ifndef RECODEX_EMPTY_STATUS_NOTIFIER_H
#define RECODEX_EMPTY_STATUS_NOTIFIER_H

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
	 * @param desc description of error which was caused in broker
	 */
	virtual void send_error(std::string desc)
	{
	}
};

#endif // RECODEX_EMPTY_STATUS_NOTIFIER_H
