#ifndef CODEX_FRONTEND_CONFIG_H
#define CODEX_FRONTEND_CONFIG_H


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

#endif // CODEX_FRONTEND_CONFIG_H
