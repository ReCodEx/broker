#include "broker_handler.h"
#include "../broker_connect.h"
#include "../notifier/reactor_status_notifier.h"

broker_handler::broker_handler(std::shared_ptr<const broker_config> config,
	std::shared_ptr<worker_registry> workers,
	std::shared_ptr<spdlog::logger> logger)
	: config_(config), workers_(workers), logger_(logger)
{
	if (logger_ == nullptr) {
		logger_ = helpers::create_null_logger();
	}

	client_commands_.register_command(
		"eval", [this](const std::string &identity, const std::vector<std::string> &message, response_cb respond) {
			process_client_eval(identity, message, respond);
		});

	worker_commands_.register_command(
		"init", [this](const std::string &identity, const std::vector<std::string> &message, response_cb respond) {
			process_worker_init(identity, message, respond);
		});

	worker_commands_.register_command(
		"done", [this](const std::string &identity, const std::vector<std::string> &message, response_cb respond) {
			process_worker_done(identity, message, respond);
		});

	worker_commands_.register_command(
		"ping", [this](const std::string &identity, const std::vector<std::string> &message, response_cb respond) {
			process_worker_ping(identity, message, respond);
		});

	worker_commands_.register_command(
		"progress", [this](const std::string &identity, const std::vector<std::string> &message, response_cb respond) {
			process_worker_progress(identity, message, respond);
		});
}

void broker_handler::on_request(const message_container &message, response_cb respond)
{
	if (message.key == broker_connect::KEY_WORKERS) {
		auto worker = workers_->find_worker_by_identity(message.identity);

		if (worker != nullptr) {
			worker->liveness = config_->get_max_worker_liveness();
			worker_timers_[worker] = std::chrono::milliseconds(0);
		}

		worker_commands_.call_function(message.data.at(0), message.identity, message.data, respond);
	}

	if (message.key == broker_connect::KEY_CLIENTS) {
		client_commands_.call_function(message.data.at(0), message.identity, message.data, respond);
	}

	if (message.key == broker_connect::KEY_TIMER) {
		process_timer(message, respond);
	}
}

void broker_handler::process_client_eval(
	const std::string &identity, const std::vector<std::string> &message, response_cb respond)
{
	// At first, let client know that we are alive and well
	respond(message_container(broker_connect::KEY_CLIENTS, identity, {"ack"}));

	// Get job identification and parse headers
	std::string job_id = message.at(1);
	request::headers_t headers;

	// Load headers terminated by an empty frame
	auto it = std::begin(message) + 2;

	while (true) {
		// End of headers
		if (it->size() == 0) {
			++it;
			break;
		}

		// Unexpected end of message - do nothing and return
		if (std::next(it) == std::end(message)) {
			logger_->warn("Unexpected end of message from frontend. Skipped.");
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
		logger_->debug(" - incomming job {}", job_id);

		// Forward remaining messages to the worker without actually understanding them
		std::vector<std::string> additional_data;
		for (; it != std::end(message); ++it) {
			additional_data.push_back(*it);
		}
		job_request_data request_data(job_id, additional_data);

		auto eval_request = std::make_shared<request>(headers, request_data);
		worker->enqueue_request(eval_request);

		if (!assign_queued_request(worker, respond)) {
			logger_->debug(" - saved to queue for worker '{}'", worker->get_description());
		}

		respond(message_container(broker_connect::KEY_CLIENTS, identity, {"accept"}));
		workers_->deprioritize_worker(worker);
	} else {
		respond(message_container(broker_connect::KEY_CLIENTS, identity, {"reject"}));
		notify_monitor(job_id, "FAILED", respond);
		logger_->error("Request '{}' rejected. No worker available for headers:", job_id);
		for (auto &header : headers) {
			logger_->error(" - {}: {}", header.first, header.second);
		}
	}
}

void broker_handler::process_worker_init(
	const std::string &identity, const std::vector<std::string> &message, response_cb respond)
{
	reactor_status_notifier status_notifier(respond, broker_connect::KEY_STATUS_NOTIFIER);

	// first let us know that message arrived (logging moved from main loop)
	logger_->debug("Received message 'init' from workers");

	// There must be at least one argument
	if (message.size() < 2) {
		logger_->warn("Init command without argument. Nothing to do.");
		return;
	}

	std::string hwgroup = message.at(1);
	request::headers_t headers;

	auto message_it = std::begin(message) + 2;
	for (; message_it != std::end(message); ++message_it) {
		auto &header = *message_it;

		if (header == "") {
			break;
		}

		size_t pos = header.find('=');
		size_t value_size = header.size() - (pos + 1);

		headers.emplace(header.substr(0, pos), header.substr(pos + 1, value_size));
	}

	// Check if we know a worker with given identity
	worker_registry::worker_ptr current_worker = workers_->find_worker_by_identity(identity);

	if (current_worker != nullptr) {
		if (current_worker->headers_equal(headers)) {
			// We don't have to update anything
			return;
		}

		status_notifier.error(
			"Received two different INIT messages from the same worker (" + current_worker->get_description() + ")");
	}


	// Create a new worker with the basic information
	auto new_worker = worker_registry::worker_ptr(new worker(identity, hwgroup, headers));

	// Load additional information
	for (; message_it != std::end(message); ++message_it) {
		auto &header = *message_it;

		size_t pos = header.find('=');
		size_t value_size = header.size() - (pos + 1);
		auto key = header.substr(0, pos);
		auto value = header.substr(pos + 1, value_size);

		if (key == "description") {
			new_worker->description = value;
		} else if (key == "current_job") {
			auto current_request = worker::request_ptr(new request(job_request_data(value)));
			new_worker->enqueue_request(current_request);
			new_worker->next_request();
		}
	}

	// Insert the worker into the registry
	workers_->add_worker(new_worker);

	// Start an idle timer for our new worker
	worker_timers_.emplace(new_worker, std::chrono::milliseconds(0));

	if (logger_->should_log(spdlog::level::debug)) {
		std::stringstream ss;
		std::copy(message.begin() + 1, message.end(), std::ostream_iterator<std::string>(ss, " "));
		logger_->debug(" - added new worker {} with headers: {}", new_worker->get_description(), ss.str());
	}
}

void broker_handler::process_worker_done(
	const std::string &identity, const std::vector<std::string> &message, response_cb respond)
{
	reactor_status_notifier status_notifier(respond, broker_connect::KEY_STATUS_NOTIFIER);

	// first let us know that message arrived (logging moved from main loop)
	logger_->debug("Received message 'done' from workers");

	worker_registry::worker_ptr worker = workers_->find_worker_by_identity(identity);

	if (worker == nullptr) {
		logger_->warn("Got 'done' message from an unknown worker");
		return;
	}

	if (message.size() < 3) {
		logger_->error("Got 'done' message with not enough arguments from worker {}", worker->get_description());
		return;
	}

	std::shared_ptr<const request> current = worker->get_current_request();

	if (message.at(1) != current->data.get_job_id()) {
		logger_->error("Got 'done' message from worker {} with mismatched job id - {} (message) vs. {} (worker)",
			worker->get_description(),
			message.at(1),
			current->data.get_job_id());
		return;
	}

	auto status = message.at(2);

	if (status == "OK") {
		// notify frontend that job ended successfully and complete it internally
		status_notifier.job_done(message.at(1));
		worker->complete_request();

		if (!assign_queued_request(worker, respond)) {
			logger_->debug(" - worker {} is now free", worker->get_description());
		}
	} else if (status == "INTERNAL_ERROR") {
		if (message.size() != 4) {
			logger_->warn(
				"Invalid number of arguments in a 'done' message with status 'INTERNAL_FAILURE' from worker {}",
				worker->get_description());
			return;
		}

		auto failed_request = worker->cancel_request();

		if (!failed_request->data.is_complete()) {
			status_notifier.rejected_job(
				failed_request->data.get_job_id(), "Job failed with '" + message.at(3) + "' and cannot be reassigned");
		} else if (check_failure_count(failed_request, status_notifier, respond)) {
			reassign_request(failed_request, respond);
		} else {
			assign_queued_request(worker, respond);
		}
	} else if (status == "FAILED") {
		if (message.size() != 4) {
			logger_->warn("Invalid number of arguments in a 'done' message with status 'FAILED' from worker {}",
				worker->get_description());
			return;
		}

		status_notifier.job_failed(message.at(1), message.at(3));

		worker->cancel_request();
		assign_queued_request(worker, respond);
	} else {
		logger_->warn("Received unexpected status code {} from worker {}", status, worker->get_description());
	}
}

void broker_handler::process_worker_ping(
	const std::string &identity, const std::vector<std::string> &message, handler_interface::response_cb respond)
{
	// first let us know that message arrived (logging moved from main loop)
	// logger_->debug() << "Received message 'ping' from workers";

	worker_registry::worker_ptr worker = workers_->find_worker_by_identity(identity);

	if (worker == nullptr) {
		respond(message_container(broker_connect::KEY_WORKERS, identity, {"intro"}));
		return;
	}

	respond(message_container(broker_connect::KEY_WORKERS, identity, {"pong"}));
}

void broker_handler::process_worker_progress(
	const std::string &identity, const std::vector<std::string> &message, handler_interface::response_cb respond)
{
	// first let us know that message arrived (logging moved from main loop)
	// logger_->debug() << "Received message 'progress' from workers";

	std::vector<std::string> monitor_message;
	monitor_message.resize(message.size() - 1);
	std::copy(message.begin() + 1, message.end(), monitor_message.begin());

	respond(message_container(broker_connect::KEY_MONITOR, broker_connect::MONITOR_IDENTITY, monitor_message));
}

void broker_handler::process_timer(const message_container &message, handler_interface::response_cb respond)
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

	reactor_status_notifier status_notifier(respond, broker_connect::KEY_STATUS_NOTIFIER);

	for (auto worker : to_remove) {
		logger_->info("Worker {} expired", worker->get_description());

		workers_->remove_worker(worker);
		auto requests = worker->terminate();
		std::vector<worker::request_ptr> unassigned_requests;

		for (auto request : *requests) {
			if (!request->data.is_complete()) {
				status_notifier.rejected_job(
					request->data.get_job_id(), "Worker timed out and its job cannot be reassigned");
				continue;
			}

			if (!check_failure_count(request, status_notifier, respond)) {
				continue;
			}

			if (!reassign_request(request, respond)) {
				unassigned_requests.push_back(request);
			} else {
				notify_monitor(request, "ABORTED", respond);
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

bool broker_handler::reassign_request(worker::request_ptr request, handler_interface::response_cb respond)
{
	logger_->debug(
		" - reassigning job {} ({} attempts already failed)", request->data.get_job_id(), request->failure_count);
	worker_registry::worker_ptr substitute_worker = workers_->find_worker(request->headers);

	if (substitute_worker == nullptr) {
		notify_monitor(request, "FAILED", respond);
		return false;
	}

	substitute_worker->enqueue_request(request);
	if (!assign_queued_request(substitute_worker, respond)) {
		logger_->debug(
			" - job {} queued for worker {}", request->data.get_job_id(), substitute_worker->get_description());
	}

	return true;
}

bool broker_handler::assign_queued_request(worker_registry::worker_ptr worker, handler_interface::response_cb respond)
{
	if (worker->next_request()) {
		respond(message_container(
			broker_connect::KEY_WORKERS, worker->identity, worker->get_current_request()->data.get()));
		logger_->debug(
			" - job {} sent to worker {}", worker->get_current_request()->data.get_job_id(), worker->get_description());
		return true;
	}

	return false;
}

bool broker_handler::check_failure_count(worker::request_ptr request, status_notifier_interface &status_notifier,
					 response_cb respond)
{
	if (request->failure_count >= config_->get_max_request_failures()) {
		status_notifier.job_failed(request->data.get_job_id(),
			"Job was reassigned too many (" + std::to_string(request->failure_count - 1) + ") times");
		notify_monitor(request, "FAILED", respond);
		return false;
	}

	return true;
}

void broker_handler::notify_monitor(worker::request_ptr request, const std::string &message,
				    handler_interface::response_cb respond) {
	notify_monitor(request->data.get_job_id(), message, respond);
}

void broker_handler::notify_monitor(const std::string &job_id, const std::string &message,
				    handler_interface::response_cb respond) {
	respond(message_container(
		broker_connect::KEY_MONITOR,
		broker_connect::MONITOR_IDENTITY,
		{job_id, message}
	));
}
