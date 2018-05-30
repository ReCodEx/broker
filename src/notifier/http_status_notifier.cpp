#include "http_status_notifier.h"

http_status_notifier::http_status_notifier(const notifier_config &config, std::shared_ptr<spdlog::logger> logger)
	: config_(config), logger_(logger)
{
	if (logger_ == nullptr) {
		logger_ = helpers::create_null_logger();
	}
}

void http_status_notifier::send(const std::string &route, const helpers::curl_params &params)
{
	std::string addr = config_.address + route;
	try {
		helpers::curl_post(addr, config_.port, params, config_.username, config_.password);
	} catch (helpers::curl_exception &e) {
		logger_->critical(e.what());
	}
}

void http_status_notifier::error(const std::string &desc)
{
	send(error_route_, {{"message", desc}});
}

void http_status_notifier::rejected_job(const std::string &job_id, const std::string &desc)
{
	rejected_jobs({job_id}, desc);
}

void http_status_notifier::rejected_jobs(std::vector<std::string> job_ids, const std::string &desc)
{
	for (auto &id : job_ids) {
		send(job_status_route_ + id, {{"status", "FAILED"}, {"message", desc}});
	}
}

void http_status_notifier::job_done(const std::string &job_id)
{
	send(job_status_route_ + job_id, {{"status", "OK"}});
}

void http_status_notifier::job_failed(const std::string &job_id, const std::string &desc)
{
	send(job_status_route_ + job_id, {{"status", "FAILED"}, {"message", desc}});
}
