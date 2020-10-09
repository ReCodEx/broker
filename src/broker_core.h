#ifndef RECODEX_BROKER_CORE_H
#define RECODEX_BROKER_CORE_H

#include "spdlog/spdlog.h"
#include <curl/curl.h>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <yaml-cpp/yaml.h>

#define BOOST_FILESYSTEM_NO_DEPRECATED
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
namespace fs = boost::filesystem;


// Our very own code includes
#include "broker_connect.h"
#include "config/broker_config.h"
#include "config/log_config.h"
#include "reactor/command_holder.h"


/**
 * Main class of whole program.
 * It handles creation and destruction of all used parts. And of course runing them.
 */
class broker_core
{
public:
	/** Disabled default constructor. */
	broker_core() = delete;

	/** Disabled copy constructor. */
	broker_core(const broker_core &source) = delete;

	/** Disabled copy assignment operator. */
	broker_core &operator=(const broker_core &source) = delete;

	/** Disabled move constructor. */
	broker_core(const broker_core &&source) = delete;

	/** Disabled move assignment operator. */
	broker_core &operator=(const broker_core &&source) = delete;

	/**
	 * There is only one constructor, which should get cmd parameters from command line.
	 * Constructor initializes all variables and structures, parses cmd parameters and loads configuration.
	 * @param args Command line parameters.
	 */
	broker_core(std::vector<std::string> args);

	/**
	 * Destructor. All structures which need to be explicitly destructed or unitialized should do it now.
	 */
	~broker_core();

	/**
	 * Constructors initializes all things,	all we have to do now is launch all the fun.
	 * This method starts broker service.
	 */
	void run();

private:
	/**
	 * Setup all things around spdlog logger, creates log path/file if not existing.
	 */
	void log_init();

	/**
	 * Construct and setup broker connection.
	 * libCURL (@a curl_init) and @ref status_notifier (@a notifier_init) should be initialized before this.
	 */
	void broker_init();

	/**
	 * Globally initializes libCURL library.
	 */
	void curl_init();

	/**
	 * libCURL cleanup.
	 */
	void curl_fini();

	/**
	 * Exit whole application with return code 1.
	 * @param msg String which is copied to stderr and logger if initialized (emerg level).
	 */
	[[noreturn]] void force_exit(const std::string &msg = "");

	/**
	 * Parse command line arguments given in constructor.
	 */
	void parse_params();

	/**
	 * Load default broker configuration from config file at default location
	 * or from file given in cmd parameters.
	 */
	void load_config();

	// PRIVATE DATA MEMBERS
	/** Command line parameters. */
	std::vector<std::string> args_;

	/** Filename where broker configuration is loaded from. */
	std::string config_filename_;

	/** Loaded broker configuration. */
	std::shared_ptr<broker_config> config_;

	/** Pointer to system logger. */
	std::shared_ptr<spdlog::logger> logger_;

	/** Pointer to task router (handles alive worker and routing tasks between them). */
	std::shared_ptr<worker_registry> workers_;

	/** Pointer to queue manager */
	std::shared_ptr<queue_manager_interface> queue_;

	/** Pointer to ZeroMQ context. */
	std::shared_ptr<zmq::context_t> context_;

	/** Main broker class which handles incoming and outgoing connections. */
	std::shared_ptr<broker_connect> broker_;
};

#endif // RECODEX_BROKER_CORE_H
