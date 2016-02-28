#include "broker_core.h"

broker_core::broker_core(std::vector<std::string> args)
	: args_(args), config_filename_("config.yml"), logger_(nullptr), broker_(nullptr)
{
	// parse cmd parameters
	parse_params();
	// load configuration from yaml file
	load_config();
	// initialize logger
	log_init();
	// construct and setup broker connection
	broker_init();
}

broker_core::~broker_core()
{
}

void broker_core::run()
{
	logger_->info() << "Broker will now start brokering.";
	broker_->start_brokering();
	logger_->info() << "Broker will now end.";
	return;
}

void broker_core::parse_params()
{
	using namespace boost::program_options;

	// Declare the supported options.
	options_description desc("Allowed options for broker");
	desc.add_options()("help,h", "Writes this help message to stderr")(
		"config,c", value<std::string>(), "Set default configuration of this program");

	variables_map vm;
	try {
		store(command_line_parser(args_).options(desc).run(), vm);
		notify(vm);
	} catch (std::exception &e) {
		force_exit("Error in loading a parameter: " + std::string(e.what()));
	}


	// Evaluate all information from command line
	if (vm.count("help")) {
		std::cerr << desc << std::endl;
		force_exit();
	}

	if (vm.count("config")) {
		config_filename_ = vm["config"].as<std::string>();
	}

	return;
}

void broker_core::load_config()
{
	try {
		YAML::Node config_yaml = YAML::LoadFile(config_filename_);
		config_ = std::make_shared<broker_config>(config_yaml);
	} catch (std::exception &e) {
		force_exit("Error loading config file: " + std::string(e.what()));
	}

	return;
}

void broker_core::force_exit(std::string msg)
{
	// write to log
	if (msg != "") {
		if (logger_ != nullptr) {
			logger_->emerg() << msg;
		}
		std::cerr << msg << std::endl;
	}

	exit(1);
}

void broker_core::log_init()
{
	auto log_conf = config_->get_log_config();

	// Set up logger
	// Try to create target directory for logs
	auto path = fs::path(log_conf.log_path);
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
		auto rotating_sink =
			std::make_shared<spdlog::sinks::rotating_file_sink_mt>((path / log_conf.log_basename).string(),
				log_conf.log_suffix,
				log_conf.log_file_size,
				log_conf.log_files_count,
				true);
		// Set queue size for asynchronous logging. It must be a power of 2.
		spdlog::set_async_mode(1048576);
		// Make log with name "logger"
		logger_ = std::make_shared<spdlog::logger>("logger", rotating_sink);
		// Set logging level to debug
		logger_->set_level(log_config::get_level(log_conf.log_level));
		// Print header to log
		logger_->emerg() << "------------------------------";
		logger_->emerg() << "    Started ReCodEx broker";
		logger_->emerg() << "------------------------------";
	} catch (spdlog::spdlog_ex &e) {
		std::cerr << "Logger: " << e.what() << std::endl;
		throw;
	}

	return;
}

void broker_core::broker_init()
{
	logger_->info() << "Initializing broker connection...";
	task_router_ = std::make_shared<task_router>();
	sockets_ = std::make_shared<connection_proxy>();

	std::shared_ptr<commands_holder<connection_proxy>> worker_cmds =
		std::make_shared<worker_commands<connection_proxy>>(sockets_, task_router_, logger_);
	std::shared_ptr<commands_holder<connection_proxy>> client_cmds =
		std::make_shared<client_commands<connection_proxy>>(sockets_, task_router_, logger_);
	broker_ = std::make_shared<broker_connect<connection_proxy>>(config_, sockets_, worker_cmds, client_cmds, logger_);
	logger_->info() << "Broker connection initialized.";

	return;
}
