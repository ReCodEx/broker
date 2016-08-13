#include "http_status_notifier.h"

http_status_notifier::http_status_notifier(const notifier_config &config, std::shared_ptr<spdlog::logger> logger)
	: config_(config), logger_(logger)
{
	if (logger_ == nullptr) {
		logger_ = helpers::create_null_logger();
	}

	address_ = config.address + ":" + std::to_string(config.port);
}

void http_status_notifier::send(std::string route, helpers::curl_params params)
{
	std::string addr = address_ + route;
	try {
		helpers::curl_post(addr, params, config_.username, config_.password);
	} catch (helpers::curl_exception e) {
		logger_->emerg() << e.what();
	}
}

void http_status_notifier::error(std::string desc)
{
	send("/error", {{"desc", desc}});
}

void http_status_notifier::rejected_jobs(std::vector<std::string> job_ids)
{
	helpers::curl_params params;
	for (auto &id : job_ids) {
		params.insert({"job_id[]", id});
	}

	send("/rejected-jobs", params);
}

void http_status_notifier::job_failed(std::string job_id)
{
	send("/job-failed", {{"job_id", job_id}});
}
