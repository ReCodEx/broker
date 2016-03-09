#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../src/worker_registry.h"

using namespace testing;

TEST(worker_registry, basic_lookup)
{
	worker_registry workers;

	auto worker1 = worker_registry::worker_ptr(new worker("id1234", "group_1", {{"env", "python"}, {"env", "c"}}));

	auto worker2 = worker_registry::worker_ptr(new worker("id12345", "group_2", {{"env", "c"}}));

	workers.add_worker(worker1);
	workers.add_worker(worker2);

	worker_registry::headers_t headers1 = {{"env", "python"}};

	worker_registry::headers_t headers2 = {{"hwgroup", "group_2"}};

	worker_registry::headers_t headers3 = {{"env", "python"}, {"hwgroup", "group_2"}};

	ASSERT_EQ(worker1, workers.find_worker(headers1));
	ASSERT_EQ(worker2, workers.find_worker(headers2));
	ASSERT_EQ(nullptr, workers.find_worker(headers3));
}

TEST(worker_registry, load_balancing)
{
	worker_registry workers;

	auto worker1 = worker_registry::worker_ptr(new worker("id1234", "group_1", {{"env", "c"}}));

	auto worker2 = worker_registry::worker_ptr(new worker("id12345", "group_1", {{"env", "c"}}));

	workers.add_worker(worker1);
	workers.add_worker(worker2);

	worker_registry::headers_t headers = {{"env", "c"}};

	auto first_found = workers.find_worker(headers);
	workers.deprioritize_worker(first_found);
	auto second_found = workers.find_worker(headers);

	ASSERT_NE(first_found, second_found);
}