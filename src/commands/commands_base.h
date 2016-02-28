#ifndef CODEX_BROKER_COMMANDS_BASE_H
#define CODEX_BROKER_COMMANDS_BASE_H

#include <memory>
#include <string>
#include <functional>
#include <map>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/null_sink.h"
#include "../task_router.h"

template <typename proxy> class command_context
{
public:
	command_context(
		std::shared_ptr<proxy> sockets, std::shared_ptr<task_router> router, std::shared_ptr<spdlog::logger> logger)
		: sockets(sockets), router(router), logger(logger)
	{
		if (this->logger == nullptr) {
			// Create logger manually to avoid global registration of logger
			auto sink = std::make_shared<spdlog::sinks::null_sink_st>();
			this->logger = std::make_shared<spdlog::logger>("command_context_nolog", sink);
			// Set loglevel to 'off' cause no logging
			this->logger->set_level(spdlog::level::off);
		}
	}
	std::shared_ptr<proxy> sockets;
	std::shared_ptr<task_router> router;
	std::shared_ptr<spdlog::logger> logger;
};


template <typename proxy> class commands_holder
{
public:
	typedef std::function<void(const std::string &, const std::vector<std::string> &, const command_context<proxy> &)>
		callback_fn;
	commands_holder(std::shared_ptr<proxy> sockets,
		std::shared_ptr<task_router> router,
		std::shared_ptr<spdlog::logger> logger = nullptr)
		: context_(sockets, router, logger)
	{
	}
	void call_function(const std::string &command, const std::string &identity, const std::vector<std::string> &message)
	{
		auto it = functions_.find(command);
		if (it != functions_.end()) {
			(it->second)(identity, message, context_);
		}
	}

protected:
	const command_context<proxy> context_;
	std::map<std::string, callback_fn> functions_;
};

#endif // CODEX_BROKER_COMMANDS_BASE_H
