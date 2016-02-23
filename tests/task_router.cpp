#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../src/task_router.h"

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
		"id12345",
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

TEST(task_router, load_balancing)
{
	task_router router;

	auto worker1 = task_router::worker_ptr(new worker(
		"id1234",
		{
			{"env", "c"}
		}
	));

	auto worker2 = task_router::worker_ptr(new worker(
		"id12345",
		{
			{"env", "c"}
		}
	));

	router.add_worker(worker1);
	router.add_worker(worker2);

	task_router::headers_t headers = {
		{"env", "c"}
	};

	auto first_found = router.find_worker(headers);
	router.deprioritize_worker(first_found);
	auto second_found = router.find_worker(headers);

	ASSERT_NE(first_found, second_found);
}