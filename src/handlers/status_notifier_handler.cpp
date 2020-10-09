#include "../helpers/curl.h"
#include "status_notifier_handler.h"

#include <map>
#include <sstream>
#include <thread>
#include <chrono>


const std::string status_notifier_handler::TYPE_ERROR = "error";
const std::string status_notifier_handler::TYPE_JOB_STATUS = "job-status";

status_notifier_handler::status_notifier_handler(const notifier_config &config, std::shared_ptr<spdlog::logger> logger)
	: config_(config), logger_(logger)
{
}

void status_notifier_handler::on_request(
	const message_container &message, const handler_interface::response_cb &respond)
{
	std::string type = "";
	std::string id = "";
	helpers::curl_params params;

	auto it = std::begin(message.data);

	while (it != std::end(message.data)) {
		const std::string &key = *it;
		const std::string &value = *(it + 1);

		if (key == "type") {
			type = value;
		} else if (key == "id") {
			id = value;
		} else {
			params[key] = value;
		}

		it += 2;
	}

	std::stringstream ss;
	ss << config_.address;

	if (type != "") {
		ss << "/" << type;
	}

	if (id != "") {
		ss << "/" << id;
	}

	std::size_t retries = 4; // we come by this value using digital suction
	auto delay = std::chrono::seconds(1);
	while (retries > 0) {
		--retries;
		try {
			helpers::curl_post(ss.str(), config_.port, params, config_.username, config_.password);
			return; // success, nothing else to try
		} catch (helpers::curl_exception &exception) {
			logger_->critical("curl failed: {}", exception.what());
			std::this_thread::sleep_for(delay);
			delay *= 2; // exponentially growing waiting intervals are cool
		}
	}
}
