#include <iostream>
#include <yaml-cpp/yaml.h>

#include "task_router.hpp"
#include "broker_config.hpp"
#include "broker.hpp"

int main (int argc, char **argv)
{
	YAML::Node yaml;

	try {
		yaml = YAML::LoadFile("config.yml");
	} catch (YAML::Exception) {
		std::cerr << "Error loading config file" << std::endl;
	}

	broker_config config(yaml);
	broker broker(config);

	broker.start_brokering();

	return 0;
}