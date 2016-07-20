#include "curl.h"

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
static size_t string_write_wrapper(void *ptr, size_t size, size_t nmemb, std::string *str)
{
	size_t length = size * nmemb;

	std::copy((char *) ptr, (char *) ptr + length, std::back_inserter(*str));

	return length;
}

std::string helpers::curl_get(
	const std::string &url, const curl_params &params, const std::string &username, const std::string &passwd)
{
	std::string result;
	std::string query = get_http_query(params);
	std::string url_query = url + "?" + query;
	CURL *curl;
	CURLcode res;

	// get curl handle
	curl = curl_easy_init();
	if (curl) {
		// destination address
		curl_easy_setopt(curl, CURLOPT_URL, url_query.c_str());

		// set writer wrapper and result string
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, string_write_wrapper);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);

		// Follow redirects
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		// Ennable support for HTTP2
		curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
		// We have trusted HTTPS certificate, so set validation on
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
		// Causes error on HTTP responses >= 400
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

		if (username.length() != 0 || passwd.length() != 0) {
			curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
			curl_easy_setopt(curl, CURLOPT_USERPWD, (username + ":" + passwd).c_str());
		}

		// Enable verbose for easier tracing
		// curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		// perform action itself
		res = curl_easy_perform(curl);

		// Always cleanup
		curl_easy_cleanup(curl);

		// Check for errors
		if (res != CURLE_OK) {
			auto error_message = "GET request failed to " + url_query + ". Error: " + curl_easy_strerror(res);
			throw curl_exception(error_message);
		}
	}

	return result;
}

std::string helpers::curl_post(
	const std::string &url, const curl_params &params, const std::string &username, const std::string &passwd)
{
	std::string result;
	std::string query = get_http_query(params);
	std::string url_query = url + "?" + query;
	CURL *curl;
	CURLcode res;

	// get curl handle
	curl = curl_easy_init();
	if (curl) {
		// destination address
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

		/* Now specify the POST data */
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query.c_str());

		// set writer wrapper and result string
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, string_write_wrapper);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);

		// Follow redirects
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		// Ennable support for HTTP2
		curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
		// We have trusted HTTPS certificate, so set validation on
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
		// Causes error on HTTP responses >= 400
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

		if (username.length() != 0 || passwd.length() != 0) {
			curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
			curl_easy_setopt(curl, CURLOPT_USERPWD, (username + ":" + passwd).c_str());
		}

		// Enable verbose for easier tracing
		// curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		// perform action itself
		res = curl_easy_perform(curl);

		// Always cleanup
		curl_easy_cleanup(curl);

		// Check for errors
		if (res != CURLE_OK) {
			auto error_message = "POST request failed to " + url_query + ". Error: " + curl_easy_strerror(res);
			throw curl_exception(error_message);
		}
	}

	return result;
}
