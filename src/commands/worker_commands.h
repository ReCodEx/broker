#ifndef RECODEX_BROKER_WORKER_COMMANDS_H
#define RECODEX_BROKER_WORKER_COMMANDS_H

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
	 * @param identity unique identification of message sender
	 * @param arguments received multipart message without leading command
	 * @param context command context of command holder
	 */
	template <typename proxy>
	void process_init(
		const std::string &identity, const std::vector<std::string> &arguments, const command_context<proxy> &context)
	{
		// first let us know that message arrived (logging moved from main loop)
		context.logger->debug() << "Received message 'init' from workers";

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
	 * Things which has to be done here is telling workers registry
	 *   that worker with given identity just finished its current job and can serve another one.
	 * @param identity unique identification of message sender
	 * @param arguments received multipart message without leading command
	 * @param context command context of command holder
	 */
	template <typename proxy>
	void process_done(
		const std::string &identity, const std::vector<std::string> &arguments, const command_context<proxy> &context)
	{
		// first let us know that message arrived (logging moved from main loop)
		context.logger->debug() << "Received message 'done' from workers";

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

		// TODO: obtain job_id better from request (current->data.at(1))
		std::shared_ptr<const request> current = worker->get_current_request();
		if (arguments.front() != current->data.at(1)) {
			context.logger->error() << "Got 'done' message with different job_id than original one from worker '"
									<< helpers::string_to_hex(identity) << "'";
			return;
		}

		// job ended with non-OK result, notify frontend about it
		if (arguments.size() == 3 && arguments.at(1) != "OK") {
			context.status_notifier->job_failed(arguments.front(), arguments.at(2));
		}

		// notify frontend that job ended successfully and complete it internally
		context.status_notifier->job_done(arguments.front());
		worker->complete_request();

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
	 * Find worker in registry and send him back "pong" message.
	 * @param identity unique identification of message sender
	 * @param arguments received multipart message without leading command
	 * @param context command context of command holder
	 */
	template <typename proxy>
	void process_ping(
		const std::string &identity, const std::vector<std::string> &arguments, const command_context<proxy> &context)
	{
		// first let us know that message arrived (logging moved from main loop)
		// context.logger->debug() << "Received message 'ping' from workers";

		worker_registry::worker_ptr worker = context.workers->find_worker_by_identity(identity);

		if (worker == nullptr) {
			context.sockets->send_workers(identity, std::vector<std::string>{"intro"});
			return;
		}

		context.sockets->send_workers(worker->identity, std::vector<std::string>{"pong"});
	}

	/**
	 * Process a "state" message from worker.
	 * Resend "state" message to monitor service.
	 * @param identity unique identification of message sender
	 * @param arguments received multipart message without leading command
	 * @param context command context of command holder
	 */
	template <typename proxy>
	void process_progress(
		const std::string &identity, const std::vector<std::string> &arguments, const command_context<proxy> &context)
	{
		// first let us know that message arrived (logging moved from main loop)
		// context.logger->debug() << "Received message 'progress' from workers";

		context.sockets->send_monitor(arguments);
	}

} // namespace


#endif // RECODEX_BROKER_WORKER_COMMANDS_H
