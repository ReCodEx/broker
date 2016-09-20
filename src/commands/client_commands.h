#ifndef RECODEX_BROKER_CLIENT_COMMANDS_H
#define RECODEX_BROKER_CLIENT_COMMANDS_H

#include "../commands/command_holder.h"
#include "../helpers/string_to_hex.h"
#include "../worker.h"


/**
 * Commands from client (frontend).
 * Commands originated from client, usually the only one frontend. See @ref command_holder for more info.
 */
namespace client_commands
{
	/**
	 * Process an "eval" request from a client.
	 * Client requested evaluation, so hand it over to proper worker with corresponding headers.
	 * "accept" or "reject" message is send back to client.
	 * @param identity unique identification of message sender
	 * @param arguments received multipart message without leading command
	 * @param context command context of command holder
	 */
	template <typename proxy>
	void process_eval(
		const std::string &identity, const std::vector<std::string> &arguments, const command_context<proxy> &context)
	{
		// At first, let client know that we are alive and well
		context.sockets->send_clients(identity, {"ack"});

		// Get job identification and parse headers
		std::string job_id = arguments.front();
		request::headers_t headers;

		// Load headers terminated by an empty frame
		auto it = std::begin(arguments) + 1;

		while (true) {
			// End of headers
			if (it->size() == 0) {
				++it;
				break;
			}

			// Unexpected end of message - do nothing and return
			if (std::next(it) == std::end(arguments)) {
				context.logger->warn() << "Unexpected end of message from frontend. Skipped.";
				return;
			}

			// Parse header, save it and continue
			size_t pos = it->find('=');
			size_t value_size = it->size() - (pos + 1);

			headers.emplace(it->substr(0, pos), it->substr(pos + 1, value_size));
			++it;
		}

		worker_registry::worker_ptr worker = context.workers->find_worker(headers);

		if (worker != nullptr) {
			context.logger->debug() << " - incomming job '" << job_id << "'";

			// Forward remaining messages to the worker without actually understanding them
			std::vector<std::string> additional_data;
			for (; it != std::end(arguments); ++it) {
				additional_data.push_back(*it);
			}
			job_request_data request_data(job_id, additional_data);

			auto eval_request = std::make_shared<request>(headers, request_data);
			worker->enqueue_request(eval_request);

			if (worker->next_request()) {
				// If the worker isn't doing anything, just forward the request
				context.sockets->send_workers(worker->identity, worker->get_current_request()->data.get());
				context.logger->debug() << " - sent to worker '" << helpers::string_to_hex(worker->identity) << "'";
			} else {
				// If the worker is occupied, queue the request
				context.logger->debug() << " - saved to queue for worker '" << helpers::string_to_hex(worker->identity)
										<< "'";
			}

			context.sockets->send_clients(identity, std::vector<std::string>{"accept"});
			context.workers->deprioritize_worker(worker);
		} else {
			context.sockets->send_clients(identity, std::vector<std::string>{"reject"});
			context.logger->error() << "Request '" << job_id << "' rejected. No worker available.";
		}
	}
} // namespace


#endif // RECODEX_BROKER_CLIENT_COMMANDS_H
