#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <ostream>

#include "../src/queuing/multi_queue_manager.h"
#include "../src/queuing/queue_manager_interface.h"
#include "mocks.h"

using namespace testing;

typedef std::multimap<std::string, std::string> worker_headers_t;

void PrintTo(const message_container &msg, std::ostream *out)
{
	*out << "{" << std::endl;
	*out << "\tkey: " << msg.key << std::endl;
	*out << "\tidentity: " << msg.identity << std::endl;
	*out << "\tmessage: ";

	for (auto it = std::begin(msg.data); it != std::end(msg.data); ++it) {
		*out << *it;
		if (std::next(it) != std::end(msg.data)) {
			*out << ", ";
		}
	}

	*out << std::endl;
	*out << "}" << std::endl;
}

TEST(broker, worker_init)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();
	auto queue = std::make_shared<multi_queue_manager>();

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// Run the tested method
	broker_handler handler(config, workers, queue, nullptr);

	handler.on_request(
		message_container(broker_connect::KEY_WORKERS, "identity1", {"init", "group_1", "env=c", "threads=8"}),
		respond);

	auto worker_1 = workers->find_worker_by_identity("identity1");

	// Exactly one worker should be present
	ASSERT_EQ(1u, workers->get_workers().size());

	// Headers should match
	ASSERT_NE(nullptr, worker_1);
	ASSERT_EQ("identity1", worker_1->identity);
	ASSERT_EQ("group_1", worker_1->hwgroup);
	ASSERT_TRUE(worker_1->check_header("env", "c"));
	ASSERT_TRUE(worker_1->check_header("threads", "8"));

	// No responses should be generated
	ASSERT_TRUE(messages.empty());
}

TEST(broker, worker_init_additional_info)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();
	auto queue = std::make_shared<multi_queue_manager>();

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// Run the tested method
	broker_handler handler(config, workers, queue, nullptr);

	handler.on_request(message_container(broker_connect::KEY_WORKERS,
						   "identity1",
						   {"init", "group_1", "env=c", "threads=8", "", "description=MyWorker", "current_job=job_42"}),
		respond);

	auto worker_1 = workers->find_worker_by_identity("identity1");

	// Exactly one worker should be present
	ASSERT_EQ(1u, workers->get_workers().size());

	// Headers should match
	ASSERT_NE(nullptr, worker_1);
	ASSERT_EQ("identity1", worker_1->identity);
	ASSERT_EQ("group_1", worker_1->hwgroup);
	ASSERT_TRUE(worker_1->check_header("env", "c"));
	ASSERT_TRUE(worker_1->check_header("threads", "8"));

	// Check the additional information
	ASSERT_EQ("MyWorker", worker_1->description);
	ASSERT_NE(nullptr, queue->get_current_request(worker_1));
	ASSERT_EQ("job_42", queue->get_current_request(worker_1)->data.get_job_id());
	ASSERT_FALSE(queue->get_current_request(worker_1)->data.is_complete());

	// No responses should be generated
	ASSERT_TRUE(messages.empty());
}

TEST(broker, worker_repeated_init_same_headers)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();
	auto queue = std::make_shared<multi_queue_manager>();

	// There is already a worker in the registry
	workers->add_worker(std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}}));

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// Run the tested method
	broker_handler handler(config, workers, queue, nullptr);

	handler.on_request(
		message_container(broker_connect::KEY_WORKERS, "identity_1", {"init", "group_1", "env=c"}), respond);

	auto worker = workers->find_worker_by_identity("identity_1");

	// Exactly one worker should be present
	ASSERT_EQ(1u, workers->get_workers().size());

	// Headers should match
	ASSERT_NE(nullptr, worker);
	ASSERT_EQ("identity_1", worker->identity);
	ASSERT_EQ("group_1", worker->hwgroup);
	ASSERT_TRUE(worker->check_header("env", "c"));

	// No responses should be generated
	ASSERT_TRUE(messages.empty());
}

TEST(broker, queuing)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();
	auto queue = std::make_shared<multi_queue_manager>();

	// There is already a worker in the registry
	auto worker_1 = std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}});
	workers->add_worker(worker_1);
	queue->add_worker(worker_1);

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, queue, nullptr);

	std::string client_id = "client_foo";

	// A client requests an evaluation
	handler.on_request(
		message_container(broker_connect::KEY_CLIENTS, client_id, {"eval", "job1", "env=c", "", "1", "2"}), respond);

	// The job should be assigned to our worker immediately
	ASSERT_THAT(messages,
		UnorderedElementsAre(
			message_container(broker_connect::KEY_WORKERS, worker_1->identity, {"eval", "job1", "1", "2"}),
			message_container(broker_connect::KEY_CLIENTS, client_id, {"ack"}),
			message_container(broker_connect::KEY_CLIENTS, client_id, {"accept"})));

	messages.clear();

	// The client requests another evaluation
	handler.on_request(
		message_container(broker_connect::KEY_CLIENTS, client_id, {"eval", "job2", "env=c", "", "1", "2"}), respond);

	// The job should be accepted, but not sent to our worker
	ASSERT_THAT(messages,
		ElementsAre(message_container(broker_connect::KEY_CLIENTS, client_id, {"ack"}),
			message_container(broker_connect::KEY_CLIENTS, client_id, {"accept"})));

	messages.clear();

	// Our worker finished the first job...
	handler.on_request(
		message_container(broker_connect::KEY_WORKERS, worker_1->identity, {"done", "job1", "OK"}), respond);

	// ...and it should get more work immediately
	ASSERT_THAT(messages,
		UnorderedElementsAre(
			message_container(broker_connect::KEY_WORKERS, worker_1->identity, {"eval", "job2", "1", "2"}),
			message_container(
				broker_connect::KEY_STATUS_NOTIFIER, "", {"type", "job-status", "id", "job1", "status", "OK"})));

	messages.clear();
}

class spying_queue_manager : public multi_queue_manager
{
public:
	std::vector<request_ptr> received_requests;

	enqueue_result enqueue_request(request_ptr request) override
	{
		received_requests.push_back(request);
		return multi_queue_manager::enqueue_request(request);
	}
};

TEST(broker, request_metadata)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();
	auto queue = std::make_shared<spying_queue_manager>();

	// There is already a worker in the registry
	auto worker_1 = std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}});
	workers->add_worker(worker_1);
	queue->add_worker(worker_1);

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, queue, nullptr);

	std::string client_id = "client_foo";

	// A client requests an evaluation
	handler.on_request(
		message_container(broker_connect::KEY_CLIENTS,
			client_id,
			{"eval", "job1", "env=c", "meta.user_id=user_asdf", "meta.exercise_id=exercise_asdf", "", "1", "2"}),
		respond);

	// The job should be assigned to our worker immediately, metadata should not be passed
	ASSERT_THAT(messages,
		UnorderedElementsAre(
			message_container(broker_connect::KEY_WORKERS, worker_1->identity, {"eval", "job1", "1", "2"}),
			message_container(broker_connect::KEY_CLIENTS, client_id, {"ack"}),
			message_container(broker_connect::KEY_CLIENTS, client_id, {"accept"})));

	ASSERT_THAT(queue->received_requests[0]->metadata,
		UnorderedElementsAre(Pair("user_id", "user_asdf"), Pair("exercise_id", "exercise_asdf")));

	messages.clear();
}

TEST(broker, freeze)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();
	auto queue = std::make_shared<multi_queue_manager>();

	// There is already a worker in the registry
	auto worker_1 = std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}});
	workers->add_worker(worker_1);
	queue->add_worker(worker_1);

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, queue, nullptr);

	std::string client_id = "client_foo";

	// A client requests an evaluation
	handler.on_request(
		message_container(broker_connect::KEY_CLIENTS, client_id, {"eval", "job1", "env=c", "", "1", "2"}), respond);

	// The job should be assigned to our worker immediately
	ASSERT_THAT(messages,
		UnorderedElementsAre(
			message_container(broker_connect::KEY_WORKERS, worker_1->identity, {"eval", "job1", "1", "2"}),
			message_container(broker_connect::KEY_CLIENTS, client_id, {"ack"}),
			message_container(broker_connect::KEY_CLIENTS, client_id, {"accept"})));

	messages.clear();

	// The broker is frozen
	handler.on_request(message_container(broker_connect::KEY_CLIENTS, client_id, {"freeze"}), respond);
	ASSERT_EQ(1u, messages.size());

	// Freeze has to be acknowledged
	ASSERT_THAT(messages, ElementsAre(message_container(broker_connect::KEY_CLIENTS, client_id, {"ack"})));

	messages.clear();

	// The client requests another evaluation
	handler.on_request(
		message_container(broker_connect::KEY_CLIENTS, client_id, {"eval", "job2", "env=c", "", "1", "2"}), respond);

	// The job should be rejected
	ASSERT_THAT(messages,
		ElementsAre(message_container(broker_connect::KEY_CLIENTS, client_id, {"ack"}),
			message_container(broker_connect::KEY_CLIENTS, client_id, {"reject", "The broker is frozen."})));

	messages.clear();
}

TEST(broker, ping_unknown_worker)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();
	auto queue = std::make_shared<multi_queue_manager>();

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, queue, nullptr);

	// A worker pings us
	handler.on_request(message_container(broker_connect::KEY_WORKERS, "identity_1", {"ping"}), respond);

	// We should ask it to introduce itself
	ASSERT_THAT(messages, ElementsAre(message_container(broker_connect::KEY_WORKERS, "identity_1", {"intro"})));

	messages.clear();

	// The worker does so with an "init" command - that's another story
}

TEST(broker, ping_known_worker)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();
	auto queue = std::make_shared<multi_queue_manager>();

	// There is already a worker in the registry
	auto worker_1 = std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}});
	workers->add_worker(worker_1);

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, queue, nullptr);

	// A worker pings us
	handler.on_request(message_container(broker_connect::KEY_WORKERS, worker_1->identity, {"ping"}), respond);

	// We know it and respond with "pong"
	ASSERT_THAT(messages, ElementsAre(message_container(broker_connect::KEY_WORKERS, "identity_1", {"pong"})));

	messages.clear();
}

TEST(broker, worker_expiration)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();
	auto queue = std::make_shared<multi_queue_manager>();

	// There is already a worker in the registry and it has a job
	auto worker_1 = std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}});
	worker_1->liveness = 1;
	auto request_1 = std::make_shared<request>(
		request::headers_t{{"env", "c"}}, request::metadata_t{{}}, job_request_data("job_id", {}));
	workers->add_worker(worker_1);
	queue->add_worker(worker_1, request_1);

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, queue, nullptr);

	// Looks like our worker timed out and there's nobody to take its work
	handler.on_request(message_container(broker_connect::KEY_TIMER, "", {"1100"}), respond);

	// We must notify the frontend and also the monitor
	ASSERT_THAT(messages,
		UnorderedElementsAre(message_container(broker_connect::KEY_STATUS_NOTIFIER,
								 "",
								 {"type",
									 "job-status",
									 "id",
									 "job_id",
									 "status",
									 "FAILED",
									 "message",
									 "Worker " + worker_1->get_description() + " dieded"}),
			message_container(broker_connect::KEY_MONITOR, broker_connect::MONITOR_IDENTITY, {"job_id", "FAILED"})));

	messages.clear();
}

TEST(broker, worker_expiration_no_job)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();
	auto queue = std::make_shared<multi_queue_manager>();

	// There is already a worker in the registry and it has a job
	auto worker_1 = std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}});
	worker_1->liveness = 1;
	workers->add_worker(worker_1);

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, queue, nullptr);

	// Looks like our worker timed out and there's nobody to take its work
	handler.on_request(message_container(broker_connect::KEY_TIMER, "", {"1100"}), respond);

	// There is no job - no messages will be necessary
	ASSERT_EQ(0u, messages.size());

	messages.clear();
}

TEST(broker, worker_state_message)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();
	auto queue = std::make_shared<multi_queue_manager>();

	// There is already a worker in the registry and it has a job
	auto worker_1 = std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}});
	auto request_1 = std::make_shared<request>(
		request::headers_t{{"env", "c"}}, request::metadata_t{{}}, job_request_data("job_id", {}));
	worker_1->liveness = 1;
	workers->add_worker(worker_1);
	queue->add_worker(worker_1, request_1);

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, queue, nullptr);

	// We got a progress message from our worker
	handler.on_request(
		message_container(broker_connect::KEY_WORKERS, worker_1->identity, {"progress", "arg1", "arg2"}), respond);

	// We should just forward it to the monitor
	ASSERT_THAT(
		messages, ElementsAre(message_container(broker_connect::KEY_MONITOR, "recodex-monitor", {"arg1", "arg2"})));
}

TEST(broker, worker_job_failed)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();
	auto queue = std::make_shared<multi_queue_manager>();

	// There is already a worker in the registry and it has a job
	auto worker_1 = std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}});
	auto request_1 = std::make_shared<request>(
		request::headers_t{{"env", "c"}}, request::metadata_t{{}}, job_request_data("job_id", {}));
	worker_1->liveness = 1;
	workers->add_worker(worker_1);
	queue->add_worker(worker_1, request_1);

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, queue, nullptr);

	// We got a message from our worker that says evaluation failed
	handler.on_request(
		message_container(
			broker_connect::KEY_WORKERS, worker_1->identity, {"done", "job_id", "FAILED", "Testing failure"}),
		respond);

	// We should notify the frontend, the monitor was notified by the worker
	ASSERT_THAT(messages,
		ElementsAre(message_container(broker_connect::KEY_STATUS_NOTIFIER,
			"",
			{"type", "job-status", "id", "job_id", "status", "FAILED", "message", "Testing failure"})));

	messages.clear();
}

TEST(broker, worker_job_failed_queueing)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();
	auto queue = std::make_shared<multi_queue_manager>();

	// There is already a worker in the registry and it has two jobs
	auto worker_1 = std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}});
	auto request_1 = std::make_shared<request>(
		request::headers_t{{"env", "c"}}, request::metadata_t{{}}, job_request_data("job_id_1", {}));
	auto request_2 = std::make_shared<request>(
		request::headers_t{{"env", "c"}}, request::metadata_t{{}}, job_request_data("job_id_2", {}));
	worker_1->liveness = 1;
	workers->add_worker(worker_1);
	queue->add_worker(worker_1);
	queue->enqueue_request(request_1);
	queue->enqueue_request(request_2);

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, queue, nullptr);

	// We got a message from our worker that says evaluation failed
	handler.on_request(message_container(broker_connect::KEY_WORKERS,
						   worker_1->identity,
						   {"done", request_1->data.get_job_id(), "FAILED", "Testing failure"}),
		respond);

	// We should notify the frontend and give the worker another job,
	// the monitor must have been notified by the worker
	ASSERT_THAT(messages,
		UnorderedElementsAre(message_container(broker_connect::KEY_STATUS_NOTIFIER,
								 "",
								 {"type",
									 "job-status",
									 "id",
									 request_1->data.get_job_id(),
									 "status",
									 "FAILED",
									 "message",
									 "Testing failure"}),
			message_container(
				broker_connect::KEY_WORKERS, worker_1->identity, {"eval", request_2->data.get_job_id()})));

	messages.clear();
}

TEST(broker, worker_job_done)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();
	auto queue = std::make_shared<multi_queue_manager>();

	// There is already a worker in the registry and it has a job
	auto worker_1 = std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}});
	auto request_1 = std::make_shared<request>(
		request::headers_t{{"env", "c"}}, request::metadata_t{{}}, job_request_data("job_id", {}));
	worker_1->liveness = 1;
	workers->add_worker(worker_1);
	queue->add_worker(worker_1, request_1);

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, queue, nullptr);

	// We got a message from our worker that says evaluation is done
	handler.on_request(
		message_container(broker_connect::KEY_WORKERS, worker_1->identity, {"done", "job_id", "OK", ""}), respond);

	// We should notify the frontend
	ASSERT_THAT(messages,
		ElementsAre(message_container(
			broker_connect::KEY_STATUS_NOTIFIER, "", {"type", "job-status", "id", "job_id", "status", "OK"})));

	messages.clear();
}

TEST(broker, worker_orphan_job_done)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();
	auto queue = std::make_shared<multi_queue_manager>();

	// There is already a worker in the registry and it has a job whose headers don't know
	auto worker_1 = std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}});
	auto request_1 = std::make_shared<request>(job_request_data("job_id"));
	worker_1->liveness = 1;
	workers->add_worker(worker_1);
	queue->add_worker(worker_1, request_1);

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, queue, nullptr);

	// We got a message from our worker that says evaluation is done
	handler.on_request(
		message_container(broker_connect::KEY_WORKERS, worker_1->identity, {"done", "job_id", "OK", ""}), respond);

	// We should notify the frontend as usual
	ASSERT_THAT(messages,
		ElementsAre(message_container(
			broker_connect::KEY_STATUS_NOTIFIER, "", {"type", "job-status", "id", "job_id", "status", "OK"})));

	messages.clear();
}

TEST(broker, worker_job_internal_failure)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();
	auto queue = std::make_shared<multi_queue_manager>();

	EXPECT_CALL(*config, get_max_request_failures()).WillRepeatedly(Return(10));

	// There is already a worker in the registry and it has a job
	auto worker_1 = std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}});
	auto request_1 = std::make_shared<request>(
		request::headers_t{{"env", "c"}}, request::metadata_t{{}}, job_request_data("job_id", {}));
	worker_1->liveness = 1;
	workers->add_worker(worker_1);
	queue->add_worker(worker_1, request_1);

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, queue, nullptr);

	// We got a message from our worker that says evaluation failed
	handler.on_request(
		message_container(
			broker_connect::KEY_WORKERS, worker_1->identity, {"done", "job_id", "INTERNAL_ERROR", "Blabla"}),
		respond);

	// An internal failure should result in the job being reassigned,
	// the monitor should have been notified by the worker
	ASSERT_THAT(messages,
		ElementsAre(message_container(
			broker_connect::KEY_WORKERS, worker_1->identity, {"eval", request_1->data.get_job_id()})));

	ASSERT_EQ(1u, request_1->failure_count);

	messages.clear();
}

TEST(broker, worker_orphan_job_internal_failure)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();
	auto queue = std::make_shared<multi_queue_manager>();

	EXPECT_CALL(*config, get_max_request_failures()).WillRepeatedly(Return(10));

	// There is already a worker in the registry and it has a job whose headers we don't know
	auto worker_1 = std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}});
	auto request_1 = std::make_shared<request>(job_request_data("job_id"));
	worker_1->liveness = 1;
	workers->add_worker(worker_1);
	queue->add_worker(worker_1, request_1);

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, queue, nullptr);

	// We got a message from our worker that says evaluation failed
	handler.on_request(
		message_container(
			broker_connect::KEY_WORKERS, worker_1->identity, {"done", "job_id", "INTERNAL_ERROR", "Testing failure"}),
		respond);

	// We cannot reassign the job (we don't know its headers). Let's just report it as failed.
	// The monitor should have been notified by the worker.
	ASSERT_THAT(messages,
		ElementsAre(message_container(broker_connect::KEY_STATUS_NOTIFIER,
			"",
			{"type",
				"job-status",
				"id",
				request_1->data.get_job_id(),
				"status",
				"FAILED",
				"message",
				"Job failed with 'Testing failure' and cannot be reassigned"})));

	messages.clear();
}

TEST(broker, worker_expiration_reassign_job)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();
	auto queue = std::make_shared<multi_queue_manager>();

	// There are two workers in the registry, one of them has a job and will die
	auto worker_1 = std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}});
	auto request_1 = std::make_shared<request>(
		request::headers_t{{"env", "c"}}, request::metadata_t{{}}, job_request_data("job_id", {"whatever"}));
	auto worker_2 = std::make_shared<worker>("identity_2", "group_1", worker_headers_t{{"env", "c"}});
	worker_1->liveness = 1;
	worker_2->liveness = 100;
	workers->add_worker(worker_1);
	workers->add_worker(worker_2);
	queue->add_worker(worker_1, request_1);
	queue->add_worker(worker_2);

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, queue, nullptr);

	handler.on_request(message_container(broker_connect::KEY_TIMER, "", {"1100"}), respond);

	// Looks like our worker timed out - the other one should get its job.
	// We must notify the monitor - the worker might not have managed to do it.
	ASSERT_THAT(messages,
		UnorderedElementsAre(
			message_container(
				broker_connect::KEY_WORKERS, worker_2->identity, {"eval", request_1->data.get_job_id(), "whatever"}),
			message_container(broker_connect::KEY_MONITOR, broker_connect::MONITOR_IDENTITY, {"job_id", "ABORTED"})));

	ASSERT_EQ(1u, request_1->failure_count);

	messages.clear();
}

TEST(broker, worker_expiration_dont_reassign_orphan_job)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();
	auto queue = std::make_shared<multi_queue_manager>();

	// There are two workers in the registry, one of them has an orphan job and will die
	auto worker_1 = std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}});
	auto request_1 = std::make_shared<request>(job_request_data("job_id"));
	auto worker_2 = std::make_shared<worker>("identity_2", "group_1", worker_headers_t{{"env", "c"}});
	worker_1->liveness = 1;
	worker_2->liveness = 100;
	workers->add_worker(worker_1);
	workers->add_worker(worker_2);
	queue->add_worker(worker_1, request_1);
	queue->add_worker(worker_2);

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, queue, nullptr);

	// Looks like our worker timed out - the other one cannot get its job because we don't know the job's headers.
	// We'll report it as failed instead. We must also notify the monitor.
	handler.on_request(message_container(broker_connect::KEY_TIMER, "", {"1100"}), respond);

	ASSERT_THAT(messages,
		ElementsAre(message_container(broker_connect::KEY_STATUS_NOTIFIER,
						"",
						{"type",
							"job-status",
							"id",
							request_1->data.get_job_id(),
							"status",
							"FAILED",
							"message",
							"Worker timed out and its job cannot be reassigned"}),
			message_container(broker_connect::KEY_MONITOR,
				broker_connect::MONITOR_IDENTITY,
				{request_1->data.get_job_id(), "FAILED"})));

	messages.clear();
}

TEST(broker, worker_expiration_cancel_job)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();
	auto queue = std::make_shared<multi_queue_manager>();

	EXPECT_CALL(*config, get_max_request_failures()).WillRepeatedly(Return(1));

	// There are two workers in the registry, one of them has a job that expired too many times
	auto worker_1 = std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}});
	auto request_1 = std::make_shared<request>(
		request::headers_t{{"env", "c"}}, request::metadata_t{{}}, job_request_data("job_id", {"whatever"}));
	request_1->failure_count = 1;
	auto worker_2 = std::make_shared<worker>("identity_2", "group_1", worker_headers_t{{"env", "c"}});
	worker_1->liveness = 1;
	worker_2->liveness = 100;
	workers->add_worker(worker_1);
	workers->add_worker(worker_2);
	queue->add_worker(worker_1, request_1);
	queue->add_worker(worker_2);

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, queue, nullptr);

	handler.on_request(message_container(broker_connect::KEY_TIMER, "", {"1100"}), respond);

	// Looks like our worker timed out and there's nobody to take its work.
	// We report it as failed and notify both frontend and monitor.
	ASSERT_THAT(messages,
		UnorderedElementsAre(
			message_container(broker_connect::KEY_STATUS_NOTIFIER,
				"",
				{"type",
					"job-status",
					"id",
					"job_id",
					"status",
					"FAILED",
					"message",
					"Job was reassigned too many (1) times. Last failure message was: Worker timed out "
					"and its job cannot be reassigned"}),
			message_container(broker_connect::KEY_MONITOR, broker_connect::MONITOR_IDENTITY, {"job_id", "FAILED"})));

	messages.clear();
}
