#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../src/worker.h"

using namespace testing;

TEST(worker, headers_basic)
{
	std::multimap<std::string, std::string> headers = {{"env", "c"}, {"threads", "8"}};

	worker worker_1("identity1", "group_1", headers);

	ASSERT_TRUE(worker_1.check_header("env", "c"));
	ASSERT_TRUE(worker_1.check_header("threads", "8"));
	ASSERT_TRUE(worker_1.check_header("hwgroup", "group_1"));

	ASSERT_FALSE(worker_1.check_header("hwgroup", "group_2"));
	ASSERT_FALSE(worker_1.check_header("time_measurement", "extra_precise"));
	ASSERT_FALSE(worker_1.check_header("threads", "10"));
}

TEST(worker, worker_headers_threads)
{
	std::multimap<std::string, std::string> headers = {{"threads", "8"}};

	worker worker_1("identity1", "group_1", headers);

	ASSERT_TRUE(worker_1.check_header("threads", "8"));
	ASSERT_TRUE(worker_1.check_header("threads", "7"));
	ASSERT_TRUE(worker_1.check_header("threads", "1"));

	ASSERT_FALSE(worker_1.check_header("threads", "10"));
}

TEST(worker, worker_headers_hwgroup)
{
	std::multimap<std::string, std::string> headers = {};

	worker worker_1("identity1", "group_1", headers);

	ASSERT_TRUE(worker_1.check_header("hwgroup", "group_1"));
	ASSERT_TRUE(worker_1.check_header("hwgroup", "group_1|group_2|group_3"));
	ASSERT_TRUE(worker_1.check_header("hwgroup", "group_2|group_1|group_3"));
	ASSERT_TRUE(worker_1.check_header("hwgroup", "group_2|group_3|group_1"));

	ASSERT_FALSE(worker_1.check_header("hwgroup", "group_2|group_3|group_4"));
	ASSERT_FALSE(worker_1.check_header("hwgroup", ""));
	ASSERT_FALSE(worker_1.check_header("hwgroup", "group_4"));
	ASSERT_FALSE(worker_1.check_header("hwgroup", "group_4||group_3"));
}

TEST(worker, worker_headers_equal)
{
	std::multimap<std::string, std::string> headers = {{"threads", "8"}};
	std::multimap<std::string, std::string> headers_copy = {{"threads", "8"}};

	worker worker_1("identity1", "group_1", headers);

	ASSERT_TRUE(worker_1.headers_equal(headers_copy));
}

TEST(worker, description)
{
	std::multimap<std::string, std::string> headers = {{"threads", "8"}};

	worker worker_1("identity1", "group_1", headers);

	ASSERT_EQ("6964656e7469747931", worker_1.get_description());

	worker_1.description = "MyWorker";

	ASSERT_EQ("6964656e7469747931 (MyWorker)", worker_1.get_description());
}
