#ifndef RECODEX_BROKER_COMMANDS_BASE_H
#define RECODEX_BROKER_COMMANDS_BASE_H

#include "../helpers/logger.h"
#include "../notifier/empty_status_notifier.h"
#include "../notifier/status_notifier.h"
#include "../worker_registry.h"
#include <functional>
#include <map>
#include <memory>
#include <string>


/**
 * Context for all commands.This class is templated by used communication proxy (mainly @ref connection_proxy class).
 * For more info, see @ref command_holder class.
 */
template <typename proxy> class command_context
{
public:
	/**
	 * Constructor with initialization of all members.
	 */
	command_context(std::shared_ptr<proxy> sockets,
		std::shared_ptr<worker_registry> workers,
		std::shared_ptr<spdlog::logger> logger)
		: sockets(sockets), workers(workers), logger(logger)
	{
		if (this->logger == nullptr) {
			this->logger = helpers::create_null_logger();
		}
	}
	/** Pointer to communication proxy (see @ref connection_proxy for possible implementation. */
	std::shared_ptr<proxy> sockets;
	/** Pointer to @ref worker_registry class - info about workers and routing preferences to them. */
	std::shared_ptr<worker_registry> workers;
	/** System logger. */
	std::shared_ptr<spdlog::logger> logger;
};


/**
 * Command holder.
 *
 * This class can handle, register and execute commands with corresponding callbacks. Command is @a std::string,
 * callback needs to be "void callback(const std::string &worker_identity, const std::vector<std::string> &args,
 * const command_context<proxy> &context". @a proxy is templated argument, which specifies type of used communication
 * class.
 */
template <typename proxy> class command_holder
{
public:
	/** Type of callback function for easier use. */
	typedef std::function<void(const std::string &, const std::vector<std::string> &, const command_context<proxy> &)>
		callback_fn;
	/** Constructor with initialization of all members. */
	command_holder(std::shared_ptr<proxy> sockets,
		std::shared_ptr<worker_registry> router,
		std::shared_ptr<spdlog::logger> logger = nullptr)
		: context_(sockets, router, logger)
	{
	}
	/**
	 * Invoke registered callback for given command (if any).
	 * @param command String reprezentation of command for processing.
	 * @param identity Unique ID of each worker for connection subsystem.
	 * @param message Arguments for callback function.
	 */
	void call_function(const std::string &command, const std::string &identity, const std::vector<std::string> &message)
	{
		auto it = functions_.find(command);
		if (it != functions_.end()) {
			(it->second)(identity, message, context_);
		}
	}
	/**
	 * Register new command with a callback.
	 * @param command String reprezentation of command.
	 * @param callback Function to call when this command occurs.
	 * @return @a true if add successful, @a false othewise
	 */
	bool register_command(const std::string &command, callback_fn callback)
	{
		auto ret = functions_.emplace(command, callback);
		return ret.second;
	}

protected:
	/** Container for <command, callback> pairs with fast searching. */
	std::map<std::string, callback_fn> functions_;

private:
	/** Inner context to be passed to callback when invoked. */
	const command_context<proxy> context_;
};

#endif // RECODEX_BROKER_COMMANDS_BASE_H
