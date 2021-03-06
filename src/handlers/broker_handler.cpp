#include "broker_handler.h"

#include "../broker_connect.h"
#include "../notifier/reactor_status_notifier.h"
#include <memory>

broker_handler::broker_handler(std::shared_ptr<const broker_config> config,
	std::shared_ptr<worker_registry> workers,
	std::shared_ptr<queue_manager_interface> queue,
	std::shared_ptr<spdlog::logger> logger)
	: config_(config), workers_(workers), queue_(queue), logger_(logger)
{
	if (logger_ == nullptr) {
		logger_ = helpers::create_null_logger();
	}

	runtime_stats_.emplace(STATS_QUEUED_JOBS, 0);
	runtime_stats_.emplace(STATS_EVALUATED_JOBS, 0);
	runtime_stats_.emplace(STATS_FAILED_JOBS, 0);
	runtime_stats_.emplace(STATS_WORKER_COUNT, 0);
	runtime_stats_.emplace(STATS_IDLE_WORKER_COUNT, 0);

	client_commands_.register_command(
		"eval", [this](const std::string &identity, const std::vector<std::string> &message, response_cb respond) {
			process_client_eval(identity, message, respond);
		});

	client_commands_.register_command("get-runtime-stats",
		[this](const std::string &identity, const std::vector<std::string> &message, response_cb respond) {
			process_client_get_runtime_stats(identity, message, respond);
		});

	client_commands_.register_command(
		"freeze", [this](const std::string &identity, const std::vector<std::string> &message, response_cb respond) {
			process_client_freeze(identity, message, respond);
		});

	client_commands_.register_command(
		"unfreeze", [this](const std::string &identity, const std::vector<std::string> &message, response_cb respond) {
			process_client_unfreeze(identity, message, respond);
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

void broker_handler::on_request(const message_container &message, const response_cb &respond)
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
	const std::string &identity, const std::vector<std::string> &message, const response_cb &respond)
{
	// first let us know that message arrived (logging moved from main loop)
	logger_->info("Received message 'eval' from clients");

	// Then let client know that we are alive and well
	respond(message_container(broker_connect::KEY_CLIENTS, identity, {"ack"}));

	// Get job identification and parse headers
	std::string job_id = message.at(1);
	request::headers_t headers;

	request::metadata_t metadata;
	const static std::string metadata_key_prefix = "meta.";

	// Load headers terminated by an empty frame
	auto it = std::begin(message) + 2;

	while (true) {
		// End of headers
		if (it->empty()) {
			++it;
			break;
		}

		// Unexpected end of message - do nothing and return
		if (std::next(it) == std::end(message)) {
			logger_->warn("Unexpected end of message from frontend. Skipped.");
			return;
		}

		// Parse header, save it and continue
		std::size_t pos = it->find('=');
		std::size_t value_size = it->size() - (pos + 1);

		const auto &key = it->substr(0, pos);
		const auto &value = it->substr(pos + 1, value_size);

		// Headers that start with the metadata prefix get stored in the separate metadata field
		if (key.substr(0, metadata_key_prefix.size()) == metadata_key_prefix) {
			metadata.emplace(key.substr(metadata_key_prefix.size()), value);
		} else {
			headers.emplace(key, value);
		}

		++it;
	}

	// If the broker is frozen, reject the request
	if (is_frozen_) {
		std::string reject_message = "The broker is frozen.";
		respond(message_container(broker_connect::KEY_CLIENTS, identity, {"reject", reject_message}));
		logger_->error("Request '{}' rejected. {}", job_id, reject_message);
		return;
	}

	// Create a job request object
	// Forward remaining messages to the worker without actually understanding them
	std::vector<std::string> additional_data;
	for (; it != std::end(message); ++it) {
		additional_data.push_back(*it);
	}

	job_request_data request_data(job_id, additional_data);
	logger_->debug(" - incoming job {}", job_id);

	auto eval_request = std::make_shared<request>(headers, metadata, request_data);
	enqueue_result result = queue_->enqueue_request(eval_request);

	if (result.enqueued) {
		if (result.assigned_to != nullptr) {
			send_request(result.assigned_to, eval_request, respond);
		} else {
			logger_->debug(" - saved to queue");
		}

		respond(message_container(broker_connect::KEY_CLIENTS, identity, {"accept"}));
	} else {
		std::string reject_message = "No worker available for given headers: ";
		logger_->error("Request '{}' rejected. No worker available for headers:", job_id);
		for (auto &header : headers) {
			reject_message += header.first + "=" + header.second + "; ";
			logger_->error(" - {}: {}", header.first, header.second);
		}

		respond(message_container(broker_connect::KEY_CLIENTS, identity, {"reject", reject_message}));
		notify_monitor(job_id, "FAILED", respond);
	}
}

void broker_handler::process_worker_init(
	const std::string &identity, const std::vector<std::string> &message, const response_cb &respond)
{
	reactor_status_notifier status_notifier(respond, broker_connect::KEY_STATUS_NOTIFIER);

	// first let us know that message arrived (logging moved from main loop)
	logger_->info("Received message 'init' from workers");

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

		std::size_t pos = header.find('=');
		std::size_t value_size = header.size() - (pos + 1);

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
	auto new_worker = std::make_shared<worker>(identity, hwgroup, headers);
	std::shared_ptr<request> current_request = nullptr;

	// Load additional information
	for (; message_it != std::end(message); ++message_it) {
		auto &header = *message_it;

		std::size_t pos = header.find('=');
		std::size_t value_size = header.size() - (pos + 1);
		auto key = header.substr(0, pos);
		auto value = header.substr(pos + 1, value_size);

		if (key == "description") {
			new_worker->description = value;
		} else if (key == "current_job") {
			current_request = std::make_shared<request>(job_request_data(value));
		}
	}

	// Insert the worker into the registry
	workers_->add_worker(new_worker);
	request_ptr request = queue_->add_worker(new_worker, current_request);

	// Give the worker a job if necessary
	if (request != nullptr) {
		send_request(new_worker, request, respond);
	}

	// Start an idle timer for our new worker
	worker_timers_.emplace(new_worker, std::chrono::milliseconds(0));

	if (logger_->should_log(spdlog::level::debug)) {
		std::stringstream ss;
		std::copy(message.begin() + 1, message.end(), std::ostream_iterator<std::string>(ss, " "));
		logger_->debug(" - added new worker {} with headers: {}", new_worker->get_description(), ss.str());
	}
}

void broker_handler::process_worker_done(
	const std::string &identity, const std::vector<std::string> &message, const response_cb &respond)
{
	reactor_status_notifier status_notifier(respond, broker_connect::KEY_STATUS_NOTIFIER);

	// first let us know that message arrived (logging moved from main loop)
	logger_->info("Received message 'done' from workers");

	worker_registry::worker_ptr worker = workers_->find_worker_by_identity(identity);

	if (worker == nullptr) {
		logger_->warn("Got 'done' message from an unknown worker");
		return;
	}

	if (message.size() < 3) {
		logger_->error("Got 'done' message with not enough arguments from worker {}", worker->get_description());
		return;
	}

	request_ptr current = queue_->get_current_request(worker);

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
		request_ptr next_request = queue_->worker_finished(worker);

		if (next_request != nullptr) {
			send_request(worker, next_request, respond);
		}

		runtime_stats_[STATS_EVALUATED_JOBS] += 1;
	} else if (status == "INTERNAL_ERROR") {
		if (message.size() != 4) {
			logger_->warn(
				"Invalid number of arguments in a 'done' message with status 'INTERNAL_FAILURE' from worker {}",
				worker->get_description());
			return;
		}

		auto failed_request = queue_->worker_cancelled(worker);
		failed_request->failure_count += 1;

		if (!failed_request->data.is_complete()) {
			status_notifier.rejected_job(
				failed_request->data.get_job_id(), "Job failed with '" + message.at(3) + "' and cannot be reassigned");
		} else if (check_failure_count(failed_request, status_notifier, respond, message.at(3))) {
			reassign_request(failed_request, respond);
		} else {
			auto new_request = queue_->assign_request(worker);
			if (new_request) {
				send_request(worker, new_request, respond);
			}
		}

		runtime_stats_[STATS_FAILED_JOBS] += 1;
	} else if (status == "FAILED") {
		if (message.size() != 4) {
			logger_->warn("Invalid number of arguments in a 'done' message with status 'FAILED' from worker {}",
				worker->get_description());
			return;
		}

		status_notifier.job_failed(message.at(1), message.at(3));

		auto failed_request = queue_->worker_cancelled(worker);
		failed_request->failure_count += 1;
		auto new_request = queue_->assign_request(worker);

		if (new_request) {
			send_request(worker, new_request, respond);
		}

		runtime_stats_[STATS_FAILED_JOBS] += 1;
	} else {
		logger_->warn("Received unexpected status code {} from worker {}", status, worker->get_description());
	}

	if (queue_->get_current_request(worker) == nullptr) {
		logger_->debug(" - worker {} is now free", worker->get_description());
	}
}

void broker_handler::process_worker_ping(
	const std::string &identity, const std::vector<std::string> &message, const handler_interface::response_cb &respond)
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
	const std::string &identity, const std::vector<std::string> &message, const handler_interface::response_cb &respond)
{
	// first let us know that message arrived (logging moved from main loop)
	// logger_->debug() << "Received message 'progress' from workers";

	std::vector<std::string> monitor_message;
	monitor_message.resize(message.size() - 1);
	std::copy(message.begin() + 1, message.end(), monitor_message.begin());

	respond(message_container(broker_connect::KEY_MONITOR, broker_connect::MONITOR_IDENTITY, monitor_message));
}

void broker_handler::process_timer(const message_container &message, const handler_interface::response_cb &respond)
{
	std::chrono::milliseconds time(std::stoll(message.data.front()));
	std::list<worker_registry::worker_ptr> to_remove;

	for (const auto &worker : workers_->get_workers()) {
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
	const static std::string failure_msg = "Worker timed out and its job cannot be reassigned";

	for (const auto &worker : to_remove) {
		logger_->info("Worker {} expired", worker->get_description());

		workers_->remove_worker(worker);
		if (queue_->get_current_request(worker) != nullptr) {
			queue_->get_current_request(worker)->failure_count += 1;
		}

		auto requests = queue_->worker_terminated(worker);
		std::vector<worker::request_ptr> unassigned_requests;

		for (const auto &request : *requests) {
			if (!request->data.is_complete()) {
				status_notifier.rejected_job(request->data.get_job_id(), failure_msg);
				notify_monitor(request, "FAILED", respond);
				continue;
			}

			if (!check_failure_count(request, status_notifier, respond, failure_msg)) {
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

			for (const auto &request : unassigned_requests) {
				status_notifier.rejected_job(request->data.get_job_id(), error_message);
			}
		}
	}

	runtime_stats_[STATS_QUEUED_JOBS] = queue_->get_queued_request_count();
	runtime_stats_[STATS_WORKER_COUNT] = workers_->get_workers().size();

	runtime_stats_[STATS_IDLE_WORKER_COUNT] = 0;
	for (auto &worker : workers_->get_workers()) {
		if (!queue_->get_current_request(worker)) {
			runtime_stats_[STATS_IDLE_WORKER_COUNT] += 1;
		}
	}

	runtime_stats_[STATS_JOBS_IN_PROGRESS] =
		runtime_stats_[STATS_WORKER_COUNT] - runtime_stats_[STATS_IDLE_WORKER_COUNT];
}

bool broker_handler::reassign_request(worker::request_ptr request, const handler_interface::response_cb &respond)
{
	logger_->debug(
		" - reassigning job {} ({} attempts already failed)", request->data.get_job_id(), request->failure_count);

	enqueue_result result = queue_->enqueue_request(request);

	if (!result.enqueued) {
		notify_monitor(request, "FAILED", respond);
		logger_->debug(" - failed to enqueue job {}", request->data.get_job_id());
		return false;
	}

	if (result.assigned_to != nullptr) {
		send_request(result.assigned_to, request, respond);
	} else {
		logger_->debug(" - job {} is now waiting in the queue", request->data.get_job_id());
	}

	return true;
}

void broker_handler::send_request(worker_registry::worker_ptr worker, request_ptr request, const response_cb &respond)
{
	respond(message_container(broker_connect::KEY_WORKERS, worker->identity, request->data.get()));
	logger_->debug(" - job {} sent to worker {}", request->data.get_job_id(), worker->get_description());
}

bool broker_handler::check_failure_count(worker::request_ptr request,
	status_notifier_interface &status_notifier,
	const response_cb &respond,
	const std::string &failure_msg)
{
	if (request->failure_count >= config_->get_max_request_failures()) {
		status_notifier.job_failed(request->data.get_job_id(),
			"Job was reassigned too many (" + std::to_string(request->failure_count - 1) +
				") times. Last failure message was: " + failure_msg);
		notify_monitor(request, "FAILED", respond);
		return false;
	}

	return true;
}

void broker_handler::notify_monitor(
	worker::request_ptr request, const std::string &message, const handler_interface::response_cb &respond)
{
	notify_monitor(request->data.get_job_id(), message, respond);
}

void broker_handler::notify_monitor(
	const std::string &job_id, const std::string &message, const handler_interface::response_cb &respond)
{
	respond(message_container(broker_connect::KEY_MONITOR, broker_connect::MONITOR_IDENTITY, {job_id, message}));
}

void broker_handler::process_client_get_runtime_stats(
	const std::string &identity, const std::vector<std::string> &message, const handler_interface::response_cb &respond)
{
	message_container response;
	response.key = broker_connect::KEY_CLIENTS;
	response.identity = identity;

	for (auto &pair : runtime_stats_) {
		response.data.push_back(pair.first);
		response.data.push_back(std::to_string(pair.second));
	}

	// additional statistics
	response.data.push_back("is-frozen");
	response.data.push_back(std::to_string(is_frozen_));

	logger_->debug("Client requested runtime statistics");
	respond(response);
}

void broker_handler::process_client_freeze(
	const std::string &identity, const std::vector<std::string> &message, const handler_interface::response_cb &respond)
{
	is_frozen_ = true;
	logger_->info("The broker was frozen and will not accept any requests until it is restarted or unfrozen");

	// let client know that freeze was successful
	respond(message_container(broker_connect::KEY_CLIENTS, identity, {"ack"}));
}

void broker_handler::process_client_unfreeze(
	const std::string &identity, const std::vector<std::string> &message, const handler_interface::response_cb &respond)
{
	is_frozen_ = false;
	logger_->info("The broker was unfrozen and will now accept requests again");

	// let client know that unfreeze was successful
	respond(message_container(broker_connect::KEY_CLIENTS, identity, {"ack"}));
}
