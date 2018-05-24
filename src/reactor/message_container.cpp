#include "message_container.h"


message_container::message_container(
	const std::string &key, const std::string &identity, const std::vector<std::string> &data)
	: key(key), identity(identity), data(data)
{
}

bool message_container::operator==(const message_container &other) const
{
	return key == other.key && identity == other.identity && data == other.data;
}
