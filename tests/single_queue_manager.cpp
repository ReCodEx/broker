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
	auto add_res = manager.add_worker(worker_1);
	ASSERT_EQ(nullptr, add_res);

	auto request_1 = std::make_shared<request>(headers, request::metadata_t{{}}, data);
	auto request_2 = std::make_shared<request>(headers, request::metadata_t{{}}, data);

	auto enq_res1 = manager.enqueue_request(request_1);
	ASSERT_EQ(worker_1, enq_res1.assigned_to);
	ASSERT_EQ(true, enq_res1.enqueued);

	auto enq_res2 = manager.enqueue_request(request_2);
	ASSERT_EQ(nullptr, enq_res2.assigned_to);
	ASSERT_EQ(true, enq_res2.enqueued);

	auto cancel_res = manager.worker_cancelled(worker_1);
	ASSERT_EQ(request_1, cancel_res);

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

TEST(single_queue_manager, rejecting_unprocessable_jobs)
{
	single_queue_manager manager;

	auto worker_1 = worker_ptr(new worker("id1234", "group_1", {{"env", "c"}}));
	auto worker_2 = worker_ptr(new worker("id12345", "group_2", {{"env", "c"}}));

	manager.add_worker(worker_1);
	manager.add_worker(worker_2);

	request::headers_t headers1 = {{"hwgroup", "group_1"}, {"env", "c"}};
	request::headers_t headers2 = {{"hwgroup", "group_2"}, {"env", "c"}};
	job_request_data data("", {});
	auto request_11 = std::make_shared<request>(headers1, request::metadata_t{{}}, data);
	auto request_12 = std::make_shared<request>(headers1, request::metadata_t{{}}, data);
	auto request_21 = std::make_shared<request>(headers2, request::metadata_t{{}}, data);
	auto request_22 = std::make_shared<request>(headers2, request::metadata_t{{}}, data);

	auto result_21 = manager.enqueue_request(request_21);
	auto result_11 = manager.enqueue_request(request_11);
	auto result_12 = manager.enqueue_request(request_12);
	auto result_22 = manager.enqueue_request(request_22);

	ASSERT_TRUE(result_11.enqueued);
	ASSERT_TRUE(result_12.enqueued);
	ASSERT_TRUE(result_21.enqueued);
	ASSERT_TRUE(result_22.enqueued);
	ASSERT_EQ(result_11.assigned_to, worker_1);
	ASSERT_EQ(result_12.assigned_to, nullptr);
	ASSERT_EQ(result_21.assigned_to, worker_2);
	ASSERT_EQ(result_22.assigned_to, nullptr);

	// termination of w1 returns all reqs for hw group_1
	ASSERT_THAT(manager.worker_terminated(worker_1), Pointee(ElementsAre(request_11, request_12)));

	// requests for hw group1 cannot be reassigned
	auto re_result_11 = manager.enqueue_request(request_11);;
	ASSERT_FALSE(re_result_11.enqueued);
	ASSERT_EQ(re_result_11.assigned_to, nullptr);
}
