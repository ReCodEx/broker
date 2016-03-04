#ifndef CODEX_BROKER_WORKER_COMMANDS_H
#define CODEX_BROKER_WORKER_COMMANDS_H

#include <sstream>


namespace worker_commands
{

	/**
	 * Process an "init" request from a worker.
	 * That means storing the identity and headers of the worker
	 * so that we can forward jobs to it.
	 */
	template <typename proxy>
	void process_init(
		const std::string &identity, const std::vector<std::string> &message, const command_context<proxy> &context)
	{
		worker_registry::headers_t headers;

		for (auto it = std::begin(message) + 1; it != std::end(message); ++it) {
			auto &header = *it;
			size_t pos = header.find('=');
			size_t value_size = header.size() - (pos + 1);

			headers.emplace(header.substr(0, pos), header.substr(pos + 1, value_size));
		}

		context.workers
			->add_worker(worker_registry::worker_ptr(new worker(identity, headers)));
		std::stringstream ss;
		std::copy(message.begin(), message.end(), std::ostream_iterator<std::string>(ss, ","));
		context.logger->debug() << "Added new worker '" << identity << "' with headers: " << ss.str();
	}

	/**
	 * Process a "done" message from a worker.
	 */
	template <typename proxy>
	void process_done(
		const std::string &identity, const std::vector<std::string> &message, const command_context<proxy> &context)
	{
		worker_registry::worker_ptr worker = context.workers
			->find_worker_by_identity(identity);

		if (worker == nullptr) {
			context.logger->warn() << "Got 'done' message from nonexisting worker";
			return;
		}

		worker->complete_request();

		if (worker->next_request()) {
			context.sockets->send_workers(worker->identity, worker->get_current_request()->data);
			context.logger->debug() << "New job sent to worker '" << identity << "'";
		} else {
			context.logger->debug() << "Worker '" << identity << "' is now free";
		}
	}

	template <typename proxy>
	void process_ping(
		const std::string &identity, const std::vector<std::string> &message, const command_context<proxy> &context)
	{
		worker_registry::worker_ptr worker = context.workers
			->find_worker_by_identity(identity);

		if (worker == nullptr) {
			context.sockets->send_workers(identity, std::vector<std::string>{"intro"});
			return;
		}

		context.sockets->send_workers(worker->identity, std::vector<std::string>{"pong"});
	}

} // namespace


#endif // CODEX_BROKER_WORKER_COMMANDS_H
