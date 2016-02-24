#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <yaml-cpp/yaml.h>

#include "../src/config/broker_config.h"

TEST(broker_config, config_basic)
{
	auto yaml = YAML::Load(
		"clients:\n"
		"    address: 192.168.5.5\n"
		"    port: 8452\n"
		"workers:\n"
		"    address: 10.0.1.2\n"
		"    port: 5482\n"
		"logger:\n"
		"    file: /var/log/isoeval\n"
		"    level: emerg\n"
		"    max-size: 2048576\n"
		"    rotations: 5\n"
	);

	broker_config config(yaml);

	log_config expected_log;
	expected_log.log_path = "/var/log";
	expected_log.log_basename = "isoeval";
	expected_log.log_level = "emerg";
	expected_log.log_file_size = 2048576;
	expected_log.log_files_count = 5;

	ASSERT_EQ("192.168.5.5", config.get_client_address());
	ASSERT_EQ(8452, config.get_client_port());
	ASSERT_EQ("10.0.1.2", config.get_worker_address());
	ASSERT_EQ(5482, config.get_worker_port());
	ASSERT_EQ(expected_log, config.get_log_config());
}

TEST(broker_config, invalid_port_1)
{
	auto yaml = YAML::Load(
		"clients:\n"
		"    port: foo\n"
	);

	ASSERT_THROW(broker_config config(yaml), config_error);
}

TEST(broker_config, invalid_port_2)
{
	auto yaml = YAML::Load(
		"clients:\n"
		"    port: 999999\n"
	);

	ASSERT_THROW(broker_config config(yaml), config_error);
}

TEST(broker_config, invalid_document)
{
	auto yaml = YAML::Load(
		"[1, 2, 3]\n"
	);

	ASSERT_THROW(broker_config config(yaml), config_error);
}
