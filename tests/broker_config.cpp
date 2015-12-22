#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <yaml-cpp/yaml.h>

#include "../src/broker_config.h"

TEST(broker_config, config_basic)
{
	auto yaml = YAML::Load(
		"client_port: 8452\n"
		"worker_port: 5482\n"
	);

	broker_config config(yaml);

	ASSERT_EQ(8452, config.get_client_port());
	ASSERT_EQ(5482, config.get_worker_port());
}

TEST(broker_config, invalid_port_1)
{
	auto yaml = YAML::Load(
		"client_port: foo\n"
	);

	ASSERT_THROW(broker_config config(yaml), config_error);
}

TEST(broker_config, invalid_port_2)
{
	auto yaml = YAML::Load(
		"client_port: 999999\n"
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
