#ifndef RECODEX_FRONTEND_CONFIG_H
#define RECODEX_FRONTEND_CONFIG_H

#include <string>


/**
 * Configuration of frontend connection and where to find it.
 */
struct frontend_config {
public:
	/**
	 * Address which frontend occupies.
	 */
	std::string address;
	/**
	 * Port on which frontend runs.
	 */
	uint16_t port;

	/**
	 * Username which is used in HTTP authentication.
	 */
	std::string username;
	/**
	 * Password for HTTP authentication.
	 */
	std::string password;
};

#endif // RECODEX_FRONTEND_CONFIG_H
