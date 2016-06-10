#include "http_status_notifier.h"

http_status_notifier::http_status_notifier(const frontend_config &config, std::shared_ptr<spdlog::logger> logger)
	: config_(config), logger_(logger)
{
	if (logger_ == nullptr) {
		logger_ = helpers::create_null_logger();
	}

	address_ = config.address + ":" + std::to_string(config.port);
}

http_status_notifier::~http_status_notifier()
{
}

void http_status_notifier::send_error(std::string desc)
{
	std::string addr = address_ + "/error";
	try {
		helpers::curl_post(addr, {{"desc", desc}}, config_.username, config_.password);
	} catch (helpers::curl_exception e) {
		logger_->emerg() << e.what();
	}
}

void http_status_notifier::send_job_done(std::string job_id)
{
	std::string addr = address_ + "/job_done";
	try {
		helpers::curl_post(addr, {{"job_id", job_id}}, config_.username, config_.password);
	} catch (helpers::curl_exception e) {
		logger_->emerg() << e.what();
	}
}
