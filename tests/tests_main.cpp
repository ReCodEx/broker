#include "gtest/gtest.h"
#include "spdlog/spdlog.h"

#define BOOST_FILESYSTEM_NO_DEPRECATED
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;


void init()
{
	//Params which will be configured - will be implemented later
	std::string log_path = "/tmp/recodex_log/";
	std::string log_basename = "broker";
	std::string log_suffix = "log";
	spdlog::level::level_enum log_level = spdlog::level::debug;
	int log_file_size = 1024 * 1024;
	int log_files_count = 3;

	//Set up logger
	//Try to create target directory for logs
	auto path = fs::path(log_path);
	try {
		if(!fs::is_directory(path)) {
			fs::create_directories(path);
		}
	} catch(fs::filesystem_error &e) {
		std::cerr << "Logger: " << e.what() << std::endl;
		throw;
	}
	//Create and register logger
	try {
		//Create multithreaded rotating file sink. Max filesize is 1024 * 1024 and we save 5 newest files.
		auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>((path / log_basename).string(),
								log_suffix, log_file_size, log_files_count, false);
		//Set queue size for asynchronous logging. It must be a power of 2.
		spdlog::set_async_mode(1048576);
		//Make log with name "logger"
		auto file_logger = std::make_shared<spdlog::logger>("logger", rotating_sink);
		//Set logging level to debug
		file_logger->set_level(log_level);
		//Register logger for global usage
		spdlog::register_logger(file_logger);
		//Print header to log
		file_logger->emerg() << "------------------------------";
		file_logger->emerg() << "   Started ReCodEx broker";
		file_logger->emerg() << "------------------------------";
	} catch(spdlog::spdlog_ex &e) {
		std::cerr << "Logger: " << e.what() << std::endl;
		throw;
	}
}


int main(int argc, char **argv) {
	try{
		init();
	} catch(...) {
		return 1;
	}
	testing::InitGoogleTest(&argc, argv);
	auto result = RUN_ALL_TESTS();

	return result;
}
