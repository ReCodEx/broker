#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../src/task_router.hpp"

using namespace testing;

TEST(task_router, basic_lookup)
{
	task_router router;

	auto worker1 = task_router::worker_ptr(new worker(
		"id1234",
		{
			{"env", "python"},
			{"env", "c"},
			{"hwgroup", "group_1"}
		}
	));

	auto worker2 = task_router::worker_ptr(new worker(
		"id1234",
		{
			{"env", "c"},
			{"hwgroup", "group_2"}
		}
	));

	router.add_worker(worker1);
	router.add_worker(worker2);

	task_router::headers_t headers1 = {
		{"env", "python"}
	};

	task_router::headers_t headers2 = {
		{"hwgroup", "group_2"}
	};

	task_router::headers_t headers3 = {
		{"env", "python"},
		{"hwgroup", "group_2"}
	};

	ASSERT_EQ(worker1, router.find_worker(headers1));
	ASSERT_EQ(worker2, router.find_worker(headers2));
	ASSERT_EQ(nullptr, router.find_worker(headers3));
}