#ifndef RECODEX_BROKER_MESSAGE_CONTAINER_H
#define RECODEX_BROKER_MESSAGE_CONTAINER_H

#include <string>
#include <vector>

struct message_container {
	/** Name of the origin or destination */
	std::string key;

	/** Identity of the peer we are communicating with (optional) */
	std::string identity = "";

	/** Frames of the message */
	std::vector<std::string> data;

	/**
	 * The default constructor
	 */
	message_container();

	/**
	 * A shorthand constructor for creating a message container in a single expression
	 * @param key name of the origin or destinarion
	 * @param identity identity of the peer we're communicating with
	 * @param data frames of the message
	 */
	message_container(const std::string &key, const std::string &identity, const std::vector<std::string> &data);

	bool operator==(const message_container &other) const;
};

#endif // RECODEX_BROKER_MESSAGE_CONTAINER_H
