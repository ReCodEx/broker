#include <gtest/gtest.h>

#include "../src/worker.h"

TEST(worker, basic_queueing)
{
	std::multimap<std::string, std::string> headers = {};
	std::vector<std::string> data = {};

	worker worker_1("identity1", headers);
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
