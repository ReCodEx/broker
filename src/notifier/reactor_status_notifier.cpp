#include "reactor_status_notifier.h"
#include "../handlers/status_notifier_handler.h"

reactor_status_notifier::reactor_status_notifier(handler_interface::response_cb callback, const std::string &key)
	: callback_(callback), key_(key)
{
}

void reactor_status_notifier::error(const std::string &desc)
{
	callback_(message_container(key_, "", {"type", status_notifier_handler::TYPE_ERROR, "message", desc}));
}

void reactor_status_notifier::rejected_job(const std::string &job_id, const std::string &desc)
{
	callback_(message_container(key_,
		"",
		{"type", status_notifier_handler::TYPE_JOB_STATUS, "id", job_id, "status", "FAILED", "message", desc}));
}

void reactor_status_notifier::rejected_jobs(std::vector<std::string> job_ids, const std::string &desc)
{
	for (auto &id : job_ids) {
		rejected_job(id, desc);
	}
}

void reactor_status_notifier::job_done(const std::string &job_id)
{
	callback_(
		message_container(key_, "", {"type", status_notifier_handler::TYPE_JOB_STATUS, "id", job_id, "status", "OK"}));
}

void reactor_status_notifier::job_failed(const std::string &job_id, const std::string &desc)
{
	callback_(message_container(key_,
		"",
		{"type", status_notifier_handler::TYPE_JOB_STATUS, "id", job_id, "status", "FAILED", "message", desc}));
}
