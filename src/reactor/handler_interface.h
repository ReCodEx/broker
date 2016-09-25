#ifndef RECODEX_BROKER_HANDLER_INTERFACE_H
#define RECODEX_BROKER_HANDLER_INTERFACE_H

#include <functional>

#include "message_container.h"

class handler_interface
{
public:
	typedef std::function<void(const message_container &)> response_cb;
	virtual void on_request(const message_container &message, response_cb response) = 0;
};

#endif // RECODEX_BROKER_HANDLER_INTERFACE_H
