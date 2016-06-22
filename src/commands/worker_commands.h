#ifndef CODEX_BROKER_WORKER_COMMANDS_H
#define CODEX_BROKER_WORKER_COMMANDS_H

#include "../helpers/string_to_hex.h"
#include "../worker.h"
#include <sstream>


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
		const std::string &identity, const std::vector<std::string> &arguments, const command_context<proxy> &context)
	{
		// There must be at least on argument
		if (arguments.size() < 1) {
			context.logger->warn() << "Init command without argument. Nothing to do.";
			return;
		}

		std::string hwgroup = arguments.front();
		request::headers_t headers;

		auto headers_start = std::begin(arguments) + 1;
		for (auto it = headers_start; it != std::end(arguments); ++it) {
			auto &header = *it;
			size_t pos = header.find('=');
			size_t value_size = header.size() - (pos + 1);

			headers.emplace(header.substr(0, pos), header.substr(pos + 1, value_size));
		}

		context.workers->add_worker(worker_registry::worker_ptr(new worker(identity, hwgroup, headers)));
		std::stringstream ss;
		std::copy(arguments.begin(), arguments.end(), std::ostream_iterator<std::string>(ss, " "));
		context.logger->debug() << " - added new worker '" << helpers::string_to_hex(identity)
								<< "' with headers: " << ss.str();
	}

	/**
	 * Process a "done" message from a worker.
	 */
	template <typename proxy>
	void process_done(
		const std::string &identity, const std::vector<std::string> &arguments, const command_context<proxy> &context)
	{
		worker_registry::worker_ptr worker = context.workers->find_worker_by_identity(identity);

		if (worker == nullptr) {
			context.logger->warn() << "Got 'done' message from nonexisting worker";
			return;
		}

		if (arguments.empty()) {
			context.logger->error() << "Got 'done' message without job_id from worker '"
									<< helpers::string_to_hex(identity) << "'";
			return;
		}

		std::shared_ptr<const request> current = worker->get_current_request();
		if (arguments.front() != current->data.at(1)) {
			context.logger->error() << "Got 'done' message with different job_id than original one from worker '"
									<< helpers::string_to_hex(identity) << "'";
			return;
		}

		worker->complete_request();
		context.notifier->send_job_done(arguments.front());

		if (worker->next_request()) {
			context.sockets->send_workers(worker->identity, worker->get_current_request()->data);
			context.logger->debug() << " - new job sent to worker '" << helpers::string_to_hex(identity)
									<< "' from queue";
		} else {
			context.logger->debug() << " - worker '" << helpers::string_to_hex(identity) << "' is now free";
		}
	}

	/**
	 * Process a "ping" message from worker.
	 */
	template <typename proxy>
	void process_ping(
		const std::string &identity, const std::vector<std::string> &arguments, const command_context<proxy> &context)
	{
		worker_registry::worker_ptr worker = context.workers->find_worker_by_identity(identity);

		if (worker == nullptr) {
			context.sockets->send_workers(identity, std::vector<std::string>{"intro"});
			return;
		}

		context.sockets->send_workers(worker->identity, std::vector<std::string>{"pong"});
	}

	/**
	 * Process a "state" message from worker.
	 */
	template <typename proxy>
	void process_progress(
		const std::string &identity, const std::vector<std::string> &arguments, const command_context<proxy> &context)
	{
		context.sockets->send_monitor(arguments);
	}

} // namespace


#endif // CODEX_BROKER_WORKER_COMMANDS_H
