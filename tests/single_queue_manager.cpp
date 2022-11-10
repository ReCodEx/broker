#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>


#include "../src/queuing/single_queue_manager.h"
#include "../src/worker.h"

using namespace testing;

TEST(single_queue_manager, add_worker)
{
	std::multimap<std::string, std::string> headers = {};
	job_request_data data("", {});

	single_queue_manager manager;

	auto worker_1 = std::make_shared<worker>("identity1", "group_1", headers);
	auto request_1 = std::make_shared<request>(headers, request::metadata_t{{}}, data);

	manager.add_worker(worker_1, request_1);
	ASSERT_EQ(request_1, manager.get_current_request(worker_1));
}

TEST(single_queue_manager, basic_queueing)
{
	std::multimap<std::string, std::string> headers = {};
	job_request_data data("", {});

	single_queue_manager manager;

	auto worker_1 = std::make_shared<worker>("identity1", "group_1", headers);
	auto request_1 = std::make_shared<request>(headers, request::metadata_t{{}}, data);
	auto request_2 = std::make_shared<request>(headers, request::metadata_t{{}}, data);

	manager.add_worker(worker_1);

	enqueue_result result = manager.enqueue_request(request_1);
	ASSERT_EQ(worker_1, result.assigned_to);
	ASSERT_TRUE(result.enqueued);
	ASSERT_EQ(request_1, manager.get_current_request(worker_1));

	result = manager.enqueue_request(request_2);
	ASSERT_TRUE(result.enqueued);
	ASSERT_EQ(nullptr, result.assigned_to);
	ASSERT_EQ(request_1, manager.get_current_request(worker_1));

	request_ptr next_request = manager.worker_finished(worker_1);
	ASSERT_EQ(next_request, request_2);
	ASSERT_EQ(request_2, manager.get_current_request(worker_1));

	next_request = manager.worker_finished(worker_1);
	ASSERT_EQ(nullptr, next_request);
	ASSERT_EQ(nullptr, manager.get_current_request(worker_1));
}

TEST(single_queue_manager, terminate_basic)
{
	single_queue_manager manager;

	std::multimap<std::string, std::string> headers = {};
	job_request_data data("", {});

	auto worker_1 = std::make_shared<worker>("identity1", "group_1", headers);
	manager.add_worker(worker_1);
	auto request_1 = std::make_shared<request>(headers, request::metadata_t{{}}, data);
	auto request_2 = std::make_shared<request>(headers, request::metadata_t{{}}, data);
	auto request_3 = std::make_shared<request>(headers, request::metadata_t{{}}, data);

	manager.enqueue_request(request_1);
	manager.enqueue_request(request_2);

	ASSERT_THAT(manager.worker_terminated(worker_1), Pointee(ElementsAre(request_1, request_2)));

	// The worker terminated -> do not assign it new requests
	enqueue_result result = manager.enqueue_request(request_3);
	ASSERT_FALSE(result.enqueued);
}

TEST(single_queue_manager, terminate_no_current)
{
	single_queue_manager manager;

	std::multimap<std::string, std::string> headers = {};
	job_request_data data("", {});

	auto worker_1 = std::make_shared<worker>("identity1", "group_1", headers);
	manager.add_worker(worker_1);
	auto request_1 = std::make_shared<request>(headers, request::metadata_t{{}}, data);
	auto request_2 = std::make_shared<request>(headers, request::metadata_t{{}}, data);

	manager.enqueue_request(request_1);
	manager.enqueue_request(request_2);
	manager.worker_cancelled(worker_1);

	ASSERT_THAT(manager.worker_terminated(worker_1), Pointee(ElementsAre(request_2)));
}

TEST(single_queue_manager, terminate_empty)
{
	single_queue_manager manager;

	std::multimap<std::string, std::string> headers = {};
	job_request_data data("", {});

	auto worker_1 = std::make_shared<worker>("identity1", "group_1", headers);
	manager.add_worker(worker_1);
	auto request_1 = std::make_shared<request>(headers, request::metadata_t{{}}, data);

	manager.enqueue_request(request_1);
	manager.worker_finished(worker_1);
	ASSERT_EQ(nullptr, manager.get_current_request(worker_1));

	ASSERT_THAT(manager.worker_terminated(worker_1), Pointee(IsEmpty()));
}

TEST(single_queue_manager, load_balancing)
{
	single_queue_manager manager;

	auto worker_1 = worker_ptr(new worker("id1234", "group_1", {{"env", "c"}}));
	auto worker_2 = worker_ptr(new worker("id12345", "group_1", {{"env", "c"}}));

	manager.add_worker(worker_1);
	manager.add_worker(worker_2);

	request::headers_t headers = {{"env", "c"}};
	job_request_data data("", {});
	auto request_1 = std::make_shared<request>(headers, request::metadata_t{{}}, data);
	auto request_2 = std::make_shared<request>(headers, request::metadata_t{{}}, data);

	enqueue_result result_1 = manager.enqueue_request(request_1);
	enqueue_result result_2 = manager.enqueue_request(request_2);

	ASSERT_TRUE(result_1.enqueued);
	ASSERT_TRUE(result_2.enqueued);
	ASSERT_NE(result_1.assigned_to, nullptr);
	ASSERT_NE(result_2.assigned_to, nullptr);
	ASSERT_NE(result_1.assigned_to, result_2.assigned_to);
}
