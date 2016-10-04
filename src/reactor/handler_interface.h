#ifndef RECODEX_BROKER_HANDLER_INTERFACE_H
#define RECODEX_BROKER_HANDLER_INTERFACE_H

#include <functional>

#include "message_container.h"

/**
 * An interface for reactor event handlers.
 */
class handler_interface
{
public:
	/**
	 * Type of the callback function passed to the handler by the reactor
	 */
	typedef std::function<void(const message_container &)> response_cb;

	/**
	 * Process a message the handler is subscribed to
	 * @param message message to be processed
	 * @param response callback that enables the handler to respond
	 */
	virtual void on_request(const message_container &message, response_cb response) = 0;
};

#endif // RECODEX_BROKER_HANDLER_INTERFACE_H
