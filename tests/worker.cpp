#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../src/worker.h"

using namespace testing;

TEST(worker, basic_queueing)
{
	std::multimap<std::string, std::string> headers = {};
	job_request_data data("", {});

	worker worker_1("identity1", "group_1", headers);
	auto request_1 = std::make_shared<request>(headers, data);
	auto request_2 = std::make_shared<request>(headers, data);

	worker_1.enqueue_request(request_1);
	ASSERT_EQ(nullptr, worker_1.get_current_request());

	ASSERT_TRUE(worker_1.next_request());
	ASSERT_EQ(request_1, worker_1.get_current_request());
	ASSERT_FALSE(worker_1.next_request());

	worker_1.enqueue_request(request_2);
	ASSERT_EQ(request_1, worker_1.get_current_request());
	ASSERT_FALSE(worker_1.next_request());

	worker_1.complete_request();
	ASSERT_EQ(nullptr, worker_1.get_current_request());

	ASSERT_TRUE(worker_1.next_request());
	ASSERT_EQ(request_2, worker_1.get_current_request());

	ASSERT_FALSE(worker_1.next_request());

	worker_1.complete_request();
	ASSERT_EQ(nullptr, worker_1.get_current_request());
	ASSERT_FALSE(worker_1.next_request());
}

TEST(worker, terminate_basic)
{
	std::multimap<std::string, std::string> headers = {};
	job_request_data data("", {});

	worker worker_1("identity1", "group_1", headers);
	auto request_1 = std::make_shared<request>(headers, data);
	auto request_2 = std::make_shared<request>(headers, data);

	worker_1.enqueue_request(request_1);
	worker_1.enqueue_request(request_2);
	ASSERT_TRUE(worker_1.next_request());

	ASSERT_THAT(worker_1.terminate(), Pointee(ElementsAre(request_1, request_2)));
}

TEST(worker, terminate_no_current)
{
	std::multimap<std::string, std::string> headers = {};
	job_request_data data("", {});

	worker worker_1("identity1", "group_1", headers);
	auto request_1 = std::make_shared<request>(headers, data);
	auto request_2 = std::make_shared<request>(headers, data);

	worker_1.enqueue_request(request_1);
	worker_1.enqueue_request(request_2);

	ASSERT_THAT(worker_1.terminate(), Pointee(ElementsAre(request_1, request_2)));
}

TEST(worker, terminate_empty)
{
	std::multimap<std::string, std::string> headers = {};
	job_request_data data("", {});

	worker worker_1("identity1", "group_1", headers);
	auto request_1 = std::make_shared<request>(headers, data);

	worker_1.enqueue_request(request_1);
	ASSERT_TRUE(worker_1.next_request());
	worker_1.complete_request();
	ASSERT_EQ(nullptr, worker_1.get_current_request());
	ASSERT_FALSE(worker_1.next_request());

	ASSERT_THAT(worker_1.terminate(), Pointee(IsEmpty()));
}

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
