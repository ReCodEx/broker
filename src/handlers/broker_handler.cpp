#include "broker_handler.h"
#include "../broker_connect.h"
#include "../notifier/reactor_status_notifier.h"

broker_handler::broker_handler(std::shared_ptr<const broker_config> config,
	std::shared_ptr<worker_registry> workers,
	std::shared_ptr<spdlog::logger> logger)
	: workers_(workers), logger_(logger), config_(config)
{
	if (logger_ == nullptr) {
		logger_ = helpers::create_null_logger();
	}
}

void broker_handler::on_request(const message_container &message, response_cb respond)
{
	reactor_status_notifier status_notifier(respond, broker_connect::KEY_STATUS_NOTIFIER);

	if (message.key == broker_connect::KEY_WORKERS) {
		auto worker = workers_->find_worker_by_identity(message.identity);

		if (worker != nullptr) {
			worker->liveness = config_->get_max_worker_liveness();
			worker_timers_[worker] = std::chrono::milliseconds(0);
		}

		const std::string &command = message.data.at(0);

		if (command == "init") {
			process_worker_init(message, respond, status_notifier);
		}

		if (command == "done") {
			process_worker_done(message, respond, status_notifier);
		}

		if (command == "ping") {
			process_worker_ping(message, respond, status_notifier);
		}

		if (command == "progress") {
			process_worker_progress(message, respond, status_notifier);
		}
	}

	if (message.key == broker_connect::KEY_CLIENTS) {
		const std::string &command = message.data.at(0);

		if (command == "eval") {
			process_client_eval(message, respond, status_notifier);
		}
	}

	if (message.key == broker_connect::KEY_TIMER) {
		process_timer(message, respond, status_notifier);
	}
}

void broker_handler::process_client_eval(
	const message_container &message, response_cb respond, status_notifier_interface &status_notifier)
{
	// At first, let client know that we are alive and well
	respond(message_container(broker_connect::KEY_CLIENTS, message.identity, {"ack"}));

	// Get job identification and parse headers
	std::string job_id = message.data.at(1);
	request::headers_t headers;

	// Load headers terminated by an empty frame
	auto it = std::begin(message.data) + 2;

	while (true) {
		// End of headers
		if (it->size() == 0) {
			++it;
			break;
		}

		// Unexpected end of message - do nothing and return
		if (std::next(it) == std::end(message.data)) {
			logger_->warn() << "Unexpected end of message from frontend. Skipped.";
			return;
		}

		// Parse header, save it and continue
		size_t pos = it->find('=');
		size_t value_size = it->size() - (pos + 1);

		headers.emplace(it->substr(0, pos), it->substr(pos + 1, value_size));
		++it;
	}

	worker_registry::worker_ptr worker = workers_->find_worker(headers);

	if (worker != nullptr) {
		logger_->debug() << " - incomming job '" << job_id << "'";

		// Forward remaining messages to the worker without actually understanding them
		std::vector<std::string> additional_data;
		for (; it != std::end(message.data); ++it) {
			additional_data.push_back(*it);
		}
		job_request_data request_data(job_id, additional_data);

		auto eval_request = std::make_shared<request>(headers, request_data);
		worker->enqueue_request(eval_request);

		if (worker->next_request()) {
			// If the worker isn't doing anything, just forward the request
			respond(message_container(
				broker_connect::KEY_WORKERS, worker->identity, worker->get_current_request()->data.get()));
			logger_->debug() << " - sent to worker '" << worker->get_description() << "'";
		} else {
			// If the worker is occupied, queue the request
			logger_->debug() << " - saved to queue for worker '" << worker->get_description() << "'";
		}

		respond(message_container(broker_connect::KEY_CLIENTS, message.identity, {"accept"}));
		workers_->deprioritize_worker(worker);
	} else {
		respond(message_container(broker_connect::KEY_CLIENTS, message.identity, {"reject"}));
		logger_->error() << "Request '" << job_id << "' rejected. No worker available.";
	}
}

void broker_handler::process_worker_init(const message_container &message,
	handler_interface::response_cb respond,
	status_notifier_interface &status_notifier)
{
	// first let us know that message arrived (logging moved from main loop)
	logger_->debug() << "Received message 'init' from workers";

	// There must be at least one argument
	if (message.data.size() < 2) {
		logger_->warn() << "Init command without argument. Nothing to do.";
		return;
	}

	std::string hwgroup = message.data.at(1);
	request::headers_t headers;

	auto headers_start = std::begin(message.data) + 2;
	for (auto it = headers_start; it != std::end(message.data); ++it) {
		auto &header = *it;
		size_t pos = header.find('=');
		size_t value_size = header.size() - (pos + 1);

		headers.emplace(header.substr(0, pos), header.substr(pos + 1, value_size));
	}

	// Check if we know a worker with given identity
	worker_registry::worker_ptr current_worker = workers_->find_worker_by_identity(message.identity);

	if (current_worker != nullptr) {
		if (current_worker->headers_equal(headers)) {
			// We don't have to update anything
			return;
		}

		status_notifier.error(
			"Received two different INIT messages from the same worker (" + current_worker->get_description() + ")");
	}


	const std::shared_ptr<worker> &new_worker =
		worker_registry::worker_ptr(new worker(message.identity, hwgroup, headers));
	workers_->add_worker(new_worker);
	worker_timers_.emplace(new_worker, std::chrono::milliseconds(0));

	if (logger_->debug().is_enabled()) {
		std::stringstream ss;
		std::copy(message.data.begin() + 1, message.data.end(), std::ostream_iterator<std::string>(ss, " "));
		logger_->debug() << " - added new worker '" << new_worker->get_description() << "' with headers: " << ss.str();
	}
}

void broker_handler::process_worker_done(const message_container &message,
	handler_interface::response_cb respond,
	status_notifier_interface &status_notifier)
{
	// first let us know that message arrived (logging moved from main loop)
	logger_->debug() << "Received message 'done' from workers";

	worker_registry::worker_ptr worker = workers_->find_worker_by_identity(message.identity);

	if (worker == nullptr) {
		logger_->warn() << "Got 'done' message from nonexisting worker";
		return;
	}

	if (message.data.size() <= 1) {
		logger_->error() << "Got 'done' message without job_id from worker '" << worker->get_description() << "'";
		return;
	}

	std::shared_ptr<const request> current = worker->get_current_request();
	if (message.data.at(1) != current->data.get_job_id()) {
		logger_->error() << "Got 'done' message with different job_id than original one from worker '"
						 << worker->get_description() << "'";
		return;
	}

	// job ended with non-OK result, notify frontend about it
	if (message.data.size() == 4 && message.data.at(2) != "OK") {
		status_notifier.job_failed(message.data.at(1), message.data.at(3));
		return;
	}

	// notify frontend that job ended successfully and complete it internally
	status_notifier.job_done(message.data.at(1));
	worker->complete_request();

	if (worker->next_request()) {
		respond(message_container(
			broker_connect::KEY_WORKERS, worker->identity, worker->get_current_request()->data.get()));
		logger_->debug() << " - new job sent to worker '" << worker->get_description() << "' from queue";
	} else {
		logger_->debug() << " - worker '" << worker->get_description() << "' is now free";
	}
}

void broker_handler::process_worker_ping(const message_container &message,
	handler_interface::response_cb respond,
	status_notifier_interface &status_notifier)
{
	// first let us know that message arrived (logging moved from main loop)
	// logger_->debug() << "Received message 'ping' from workers";

	worker_registry::worker_ptr worker = workers_->find_worker_by_identity(message.identity);

	if (worker == nullptr) {
		respond(message_container(broker_connect::KEY_WORKERS, message.identity, {"intro"}));
		return;
	}

	respond(message_container(broker_connect::KEY_WORKERS, message.identity, {"pong"}));
}

void broker_handler::process_worker_progress(const message_container &message,
	handler_interface::response_cb respond,
	status_notifier_interface &status_notifier)
{
	// first let us know that message arrived (logging moved from main loop)
	// logger_->debug() << "Received message 'progress' from workers";

	std::vector<std::string> monitor_message;
	monitor_message.resize(message.data.size() - 1);
	std::copy(message.data.begin() + 1, message.data.end(), monitor_message.begin());

	respond(message_container(broker_connect::KEY_MONITOR, "", monitor_message));
}

void broker_handler::process_timer(const message_container &message,
	handler_interface::response_cb respond,
	status_notifier_interface &status_notifier)
{
	std::chrono::milliseconds time(std::stoll(message.data.front()));
	std::list<worker_registry::worker_ptr> to_remove;

	for (auto worker : workers_->get_workers()) {
		if (worker_timers_.find(worker) == std::end(worker_timers_)) {
			worker_timers_[worker] = std::chrono::milliseconds(0);
		}

		worker_timers_[worker] += time;

		if (worker_timers_[worker] > config_->get_worker_ping_interval()) {
			worker->liveness -= 1;
			worker_timers_[worker] = std::chrono::milliseconds(0);

			if (worker->liveness <= 0) {
				to_remove.push_back(worker);
			}
		}
	}

	for (auto worker : to_remove) {
		logger_->notice() << "Worker " + worker->get_description() + " expired";

		workers_->remove_worker(worker);
		auto requests = worker->terminate();
		std::vector<worker::request_ptr> unassigned_requests;

		for (auto request : *requests) {
			worker_registry::worker_ptr substitute_worker = workers_->find_worker(request->headers);

			if (substitute_worker != nullptr) {
				substitute_worker->enqueue_request(request);

				if (substitute_worker->next_request()) {
					respond(message_container(broker_connect::KEY_WORKERS,
						substitute_worker->identity,
						substitute_worker->get_current_request()->data.get()));
				}
			} else {
				unassigned_requests.push_back(request);
			}
		}

		// there are requests which cannot be assigned, notify frontend about it
		if (!unassigned_requests.empty()) {
			std::string error_message = "Worker " + worker->get_description() + " dieded";

			for (auto request : unassigned_requests) {
				status_notifier.rejected_job(request->data.get_job_id(), error_message);
			}
		}
	}
}
