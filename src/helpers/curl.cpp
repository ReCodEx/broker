#include "curl.h"
#include <memory>

// Tweak for older libcurls
#ifndef CURL_HTTP_VERSION_2_0
#define CURL_HTTP_VERSION_2_0 CURL_HTTP_VERSION_1_1
#endif


std::string helpers::get_http_query(const curl_params &params)
{
	std::string result;

	for (auto &par : params) {
		if (result.length() > 0) {
			result += "&";
		}

		result += par.first + "=" + par.second;
	}

	return result;
}

/**
 * Wrapper function for CURL for writing result into given string.
 * @param ptr base location of result
 * @param size size of result in nmemb items (size in bytes is size * nmemb)
 * @param nmemb size of one pointer item
 * @param str result string in which return body will be stored
 * @return length of data which were written into str
 */
static std::size_t string_write_wrapper(void *ptr, std::size_t size, std::size_t nmemb, std::string *str)
{
	std::size_t length = size * nmemb;

	std::copy((char *) ptr, (char *) ptr + length, std::back_inserter(*str));

	return length;
}

std::string helpers::curl_get(const std::string &url,
	const long port,
	const curl_params &params,
	const std::string &username,
	const std::string &passwd)
{
	std::string result;
	std::string query = get_http_query(params);
	std::string url_query = url + "?" + query;
	CURLcode res;

	// get curl handle
	// std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl = {curl_easy_init(), curl_easy_cleanup};
	std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl = {curl_easy_init(), curl_easy_cleanup};
	if (curl.get()) {
		// destination address
		curl_easy_setopt(curl.get(), CURLOPT_URL, url_query.c_str());

		// set port
		curl_easy_setopt(curl.get(), CURLOPT_PORT, port);

		// set writer wrapper and result string
		curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, string_write_wrapper);
		curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &result);

		// Follow redirects
		curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
		// Ennable support for HTTP2
		curl_easy_setopt(curl.get(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
		// We have trusted HTTPS certificate, so set validation on
		curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 2L);
		// Causes error on HTTP responses >= 400
		curl_easy_setopt(curl.get(), CURLOPT_FAILONERROR, 1L);

		if (username.length() != 0 || passwd.length() != 0) {
			curl_easy_setopt(curl.get(), CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
			curl_easy_setopt(curl.get(), CURLOPT_USERPWD, (username + ":" + passwd).c_str());
		}

		// Enable verbose for easier tracing
		// curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		// perform action itself
		res = curl_easy_perform(curl.get());

		// Check for errors
		if (res != CURLE_OK) {
			long response_code;
			curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &response_code);
			auto error_message = "GET request failed to " + url_query + ". Error: (" + std::to_string(response_code) +
				") " + curl_easy_strerror(res);
			throw curl_exception(error_message);
		}
	}

	return result;
}

std::string helpers::curl_post(const std::string &url,
	const long port,
	const curl_params &params,
	const std::string &username,
	const std::string &passwd)
{
	std::string result;
	std::string query = get_http_query(params);
	std::string url_query = url + "?" + query;
	CURLcode res;

	// get curl handle
	std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl = {curl_easy_init(), curl_easy_cleanup};
	if (curl.get()) {
		// destination address
		curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());

		// set port
		curl_easy_setopt(curl.get(), CURLOPT_PORT, port);

		/* Now specify the POST data */
		curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, query.c_str());

		// set writer wrapper and result string
		curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, string_write_wrapper);
		curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &result);

		// Follow redirects
		curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
		// Ennable support for HTTP2
		curl_easy_setopt(curl.get(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
		// We have trusted HTTPS certificate, so set validation on
		curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 2L);
		// Causes error on HTTP responses >= 400
		curl_easy_setopt(curl.get(), CURLOPT_FAILONERROR, 1L);

		if (username.length() != 0 || passwd.length() != 0) {
			curl_easy_setopt(curl.get(), CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
			curl_easy_setopt(curl.get(), CURLOPT_USERPWD, (username + ":" + passwd).c_str());
		}

		// Enable verbose for easier tracing
		// curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		// perform action itself
		res = curl_easy_perform(curl.get());

		// Check for errors
		if (res != CURLE_OK) {
			long response_code;
			curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &response_code);
			auto error_message = "POST request failed to " + url_query + ". Error: (" + std::to_string(response_code) +
				") " + curl_easy_strerror(res);
			throw curl_exception(error_message);
		}
	}

	return result;
}

namespace helpers
{

	curl_exception::curl_exception(const std::string &what) : what_(what)
	{
	}

	const char *curl_exception::what() const noexcept
	{
		return what_.c_str();
	}

} // namespace helpers
