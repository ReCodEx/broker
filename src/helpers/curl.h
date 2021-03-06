#ifndef RECODEX_BROKER_HELPERS_CURL_H
#define RECODEX_BROKER_HELPERS_CURL_H

#include <curl/curl.h>
#include <map>
#include <string>

namespace helpers
{
	/**
	 * Defines std::map as keeper of http parameters, aka associative array.
	 */
	using curl_params = std::map<std::string, std::string>;

	/**
	 * Construct standard http query and return it.
	 * @note Leading question mark is not included in returned string.
	 * @param params source of parameters
	 * @return textual representation of params without leading question mark
	 */
	std::string get_http_query(const curl_params &params);

	/**
	 * Sends GET request with curl to given url with given parameters.
	 * @note Http authentication will be executed when username or password will be non-empty.
	 * @param url address which will be getted
	 * @param port port of remote host
	 * @param params http query will be constructed from this
	 * @param username http authentication
	 * @param passwd http authentication
	 * @return body of response from server
	 * @throws curl_exception if request was not succesfull
	 */
	std::string curl_get(const std::string &url,
		const long port,
		const curl_params &params,
		const std::string &username = "",
		const std::string &passwd = "");

	/**
	 * Sends POST request with curl to given url with given parameters.
	 * @note Http authentication will be executed when username or password will be non-empty.
	 * @param url address which will be requested with post
	 * @param port port of remote host
	 * @param params http query will be constructed from this
	 * @param username http authentication
	 * @param passwd http authentication
	 * @return body of response from server
	 * @throws curl_exception if request was not succesfull
	 */
	std::string curl_post(const std::string &url,
		const long port,
		const curl_params &params,
		const std::string &username = "",
		const std::string &passwd = "");


	/**
	 * Special exception for curl helper functions/classes
	 */
	class curl_exception : public std::exception
	{
	public:
		/**
		 * Generic constructor.
		 */
		curl_exception() : what_("Generic curl exception")
		{
		}

		/**
		 * Constructor with further description.
		 * @param what circumstances of non-standard action
		 */
		curl_exception(const std::string &what);

		/**
		 * Stated for completion.
		 */
		~curl_exception() override = default;

		/**
		 * Returns circumstances of this exception.
		 * @return c-like textual value
		 */
		const char *what() const noexcept override;

	protected:
		/** Describes circumstances which lead to throwing this exception. */
		std::string what_;
	};
} // namespace helpers


#endif // RECODEX_BROKER_HELPERS_CURL_H
