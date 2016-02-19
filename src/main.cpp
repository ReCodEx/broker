#include <iostream>
#include <yaml-cpp/yaml.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <vector>

#define BOOST_FILESYSTEM_NO_DEPRECATED
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "task_router.h"
#include "broker_config.h"
#include "broker.h"
#include "connection_proxy.h"


namespace fs = boost::filesystem;
namespace opts = boost::program_options;


/**
 * Function for global initialization of used resources.
 */
std::shared_ptr<spdlog::logger> init_logger()
{
	// Params which will be configured - will be implemented later
	std::string log_path = "/tmp/recodex_log/";
	std::string log_basename = "broker";
	std::string log_suffix = "log";
	spdlog::level::level_enum log_level = spdlog::level::debug;
	int log_file_size = 1024 * 1024;
	int log_files_count = 3;

	// Set up logger
	// Try to create target directory for logs
	auto path = fs::path(log_path);
	try {
		if (!fs::is_directory(path)) {
			fs::create_directories(path);
		}
	} catch (fs::filesystem_error &e) {
		std::cerr << "Logger: " << e.what() << std::endl;
		throw;
	}
	// Create and register logger
	try {
		// Create multithreaded rotating file sink. Max filesize is 1024 * 1024 and we save 5 newest files.
		auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
			(path / log_basename).string(), log_suffix, log_file_size, log_files_count, false);
		// Set queue size for asynchronous logging. It must be a power of 2.
		spdlog::set_async_mode(1048576);
		// Make log with name "logger"
		auto file_logger = std::make_shared<spdlog::logger>("logger", rotating_sink);
		// Set logging level to debug
		file_logger->set_level(log_level);
		// Print header to log
		file_logger->emerg() << "------------------------------";
		file_logger->emerg() << "    Started ReCodEx broker";
		file_logger->emerg() << "------------------------------";
		return file_logger;
	} catch (spdlog::spdlog_ex &e) {
		std::cerr << "Logger: " << e.what() << std::endl;
		throw;
	}
}

struct cmd_options
{
	std::string config_filename;
};

void parse_options(const std::vector<std::string> &args, cmd_options &options)
{
	// Declare the supported options.
	opts::options_description desc("Allowed options for IsoEval");
	desc.add_options()("help,h", "Writes this help message to stderr")(
		"config,c", opts::value<std::string>(), "Set path to the configuration file");

	opts::variables_map vm;
	try {
		store(opts::command_line_parser(args).options(desc).run(), vm);
		notify(vm);
	} catch (std::exception &e) {
		std::cerr << ("Error loading command line options: " + std::string(e.what()));
		exit(1);
	}


	// Evaluate all information from command line
	if (vm.count("help")) {
		std::cout << desc << std::endl;
		exit(1);
	}

	if (vm.count("config")) {
		options.config_filename = vm["config"].as<std::string>();
	} else {
		options.config_filename = "config.yml";
	}

	return;
}

int main(int argc, char **argv)
{
	std::vector<std::string> args(argv, argv + argc);
	cmd_options options;

	parse_options(args, options);

	YAML::Node yaml;

	try {
		yaml = YAML::LoadFile(options.config_filename);
	} catch (YAML::Exception) {
		std::cerr << "Error loading config file" << std::endl;
	}

	std::shared_ptr<spdlog::logger> logger = nullptr;
	try {
		logger = init_logger();
	} catch (...) {
		return 1;
	}

	auto config = std::make_shared<broker_config>(yaml);
	auto router = std::make_shared<task_router>();
	auto sockets = std::make_shared<connection_proxy>();
	broker<connection_proxy> broker(config, sockets, router, logger);

	broker.start_brokering();

	return 0;
}
