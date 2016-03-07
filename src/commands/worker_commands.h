#ifndef CODEX_BROKER_WORKER_COMMANDS_H
#define CODEX_BROKER_WORKER_COMMANDS_H

#include <sstream>
#include "../helpers/string_to_hex.h"


/**
 * Commands from workers.
 * Commands originated from one of workers. See @ref command_holder for more info.
 */
namespace worker_commands
{
	/**
	 * Process an "init" request from a worker.
	 * That means storing the identity and headers of the worker so that we can forward jobs to it.
	 */
	template <typename proxy>
	void process_init(
		const std::string &identity, const std::vector<std::string> &message, const command_context<proxy> &context)
	{
		task_router::headers_t headers;

		for (auto it = std::begin(message) + 1; it != std::end(message); ++it) {
			auto &header = *it;
			size_t pos = header.find('=');
			size_t value_size = header.size() - (pos + 1);

			headers.emplace(header.substr(0, pos), header.substr(pos + 1, value_size));
		}

		context.router->add_worker(task_router::worker_ptr(new worker(identity, headers)));
		std::stringstream ss;
		std::copy(++message.begin(), message.end(), std::ostream_iterator<std::string>(ss, " "));
		context.logger->debug() << " - added new worker '" << helpers::string_to_hex(identity)
								<< "' with headers: " << ss.str();
	}

	/**
	 * Process a "done" message from a worker.
	 */
	template <typename proxy>
	void process_done(
		const std::string &identity, const std::vector<std::string> &message, const command_context<proxy> &context)
	{
		context.logger->debug() << " - from worker '" << helpers::string_to_hex(identity) << "'";
		task_router::worker_ptr worker = context.router->find_worker_by_identity(identity);

		if (worker == nullptr) {
			context.logger->warn() << "Got 'done' message from nonexisting worker";
			return;
		}

		if (worker->request_queue.empty()) {
			worker->free = true;
			context.logger->debug() << " - worker is now free";
		} else {
			context.sockets->send_workers(worker->identity, worker->request_queue.front());
			worker->request_queue.pop();
			context.logger->debug() << " - new job sent to worker from queue";
		}
	}
} // namespace


#endif // CODEX_BROKER_WORKER_COMMANDS_H
