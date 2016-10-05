#include "command_holder.h"

void command_holder::call_function(const std::string &command,
	const std::string &identity,
	const std::vector<std::string> &message,
	handler_interface::response_cb respond)
{
	auto it = functions_.find(command);
	if (it != functions_.end()) {
		(it->second)(identity, message, respond);
	}
}

bool command_holder::register_command(const std::string &command, command_holder::callback_fn callback)
{
	auto ret = functions_.emplace(command, callback);
	return ret.second;
}
