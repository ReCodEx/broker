#ifndef RECODEX_BROKER_COMMANDS_BASE_H
#define RECODEX_BROKER_COMMANDS_BASE_H

#include "../helpers/logger.h"
#include "../notifier/empty_status_notifier.h"
#include "../notifier/status_notifier.h"
#include "handler_interface.h"
#include "message_container.h"
#include "../worker_registry.h"
#include <functional>
#include <map>
#include <memory>
#include <string>


/**
 * Command holder.
 *
 * This class can handle, register and execute commands with corresponding callbacks. Command is @a std::string,
 * callback needs to be "void callback(const std::string &worker_identity, const std::vector<std::string> &args,
 * const command_context<proxy> &context". @a proxy is templated argument, which specifies type of used communication
 * class.
 */
class command_holder
{
public:
	/** Type of callback function for easier use. */
	typedef std::function<void(const std::string &, const std::vector<std::string> &, handler_interface::response_cb)>
		callback_fn;

	/**
	 * Invoke registered callback for given command (if any).
	 * @param command String reprezentation of command for processing.
	 * @param identity Unique ID of each worker for connection subsystem.
	 * @param message Arguments for callback function.
	 * @param respond a callback to let the handler respond
	 */
	void call_function(const std::string &command,
		const std::string &identity,
		const std::vector<std::string> &message,
		handler_interface::response_cb respond);

	/**
	 * Register new command with a callback.
	 * @param command String reprezentation of command.
	 * @param callback Function to call when this command occurs.
	 * @return @a true if add successful, @a false othewise
	 */
	bool register_command(const std::string &command, callback_fn callback);

private:
	/** Container for <command, callback> pairs with fast searching. */
	std::map<std::string, callback_fn> functions_;
};

#endif // RECODEX_BROKER_COMMANDS_BASE_H
