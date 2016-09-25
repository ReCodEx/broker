#ifndef RECODEX_BROKER_MESSAGE_CONTAINER_H
#define RECODEX_BROKER_MESSAGE_CONTAINER_H

#include <string>
#include <vector>

struct message_container {
	/** Name of the origin or destination */
	std::string key;

	/** Identity of the peer we are communicating with (optional) */
	std::string identity;

	/** Frames of the message */
	std::vector<std::string> data;
};

#endif // RECODEX_BROKER_MESSAGE_CONTAINER_H
