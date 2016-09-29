#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <ostream>

#include "mocks.h"

using namespace testing;

typedef std::multimap<std::string, std::string> worker_headers_t;

void PrintTo(const message_container &msg, std::ostream *out)
{
	*out << "key: " << msg.key << std::endl;
	*out << "identity: " << msg.identity << std::endl;
	*out << "message: ";

	for (auto it = std::begin(msg.data); it != std::end(msg.data); ++it) {
		*out << *it;
		if (std::next(it) != std::end(msg.data)) {
			*out << ", ";
		}
	}

	*out << std::endl;
}

TEST(broker, worker_init)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// Run the tested method
	broker_handler handler(config, workers, nullptr);

	handler.on_request(
		message_container(broker_connect::KEY_WORKERS, "identity1", {"init", "group_1", "env=c", "threads=8"}),
		respond);

	auto worker_1 = workers->find_worker_by_identity("identity1");

	// Exactly one worker should be present
	ASSERT_EQ(1, workers->get_workers().size());

	// Headers should match
	ASSERT_NE(nullptr, worker_1);
	ASSERT_EQ("identity1", worker_1->identity);
	ASSERT_EQ("group_1", worker_1->hwgroup);
	ASSERT_TRUE(worker_1->check_header("env", "c"));
	ASSERT_TRUE(worker_1->check_header("threads", "8"));

	// No responses should be generated
	ASSERT_TRUE(messages.empty());
}

TEST(broker, worker_repeated_init_same_headers)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();

	// There is already a worker in the registry
	workers->add_worker(std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}}));

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// Run the tested method
	broker_handler handler(config, workers, nullptr);

	handler.on_request(
		message_container(broker_connect::KEY_WORKERS, "identity_1", {"init", "group_1", "env=c"}), respond);

	auto worker = workers->find_worker_by_identity("identity_1");

	// Exactly one worker should be present
	ASSERT_EQ(1, workers->get_workers().size());

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

	// There is already a worker in the registry
	auto worker_1 = std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}});
	workers->add_worker(worker_1);

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, nullptr);

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
	handler.on_request(message_container(broker_connect::KEY_WORKERS, worker_1->identity, {"done", "job1"}), respond);

	// ...and it should get more work immediately
	ASSERT_THAT(
		messages,
		UnorderedElementsAre(
			message_container(broker_connect::KEY_WORKERS, worker_1->identity, {"eval", "job2", "1", "2"}),
			message_container(
				broker_connect::KEY_STATUS_NOTIFIER, "", {"type", "job_status", "id", "job1", "status", "OK"})));

	messages.clear();
}

TEST(broker, ping_unknown_worker)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, nullptr);

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

	// There is already a worker in the registry
	auto worker_1 = std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}});
	workers->add_worker(worker_1);

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, nullptr);

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

	// There is already a worker in the registry and it has a job
	auto worker_1 = std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}});
	auto request_1 = std::make_shared<request>(request::headers_t{{"env", "c"}}, job_request_data("job_id", {}));
	worker_1->liveness = 1;
	worker_1->enqueue_request(request_1);
	ASSERT_TRUE(worker_1->next_request());
	workers->add_worker(worker_1);

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, nullptr);

	// Looks like our worker timed out and there's nobody to take its work
	handler.on_request(message_container(broker_connect::KEY_TIMER, "", {"1100"}), respond);

	ASSERT_THAT(messages,
		UnorderedElementsAre(message_container(broker_connect::KEY_STATUS_NOTIFIER,
			"",
			{"type",
				"job_status",
				"id",
				"job_id",
				"status",
				"FAILED",
				"message",
				"Worker " + worker_1->get_description() + " dieded"})));

	messages.clear();
}

TEST(broker, worker_state_message)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();

	// There is already a worker in the registry and it has a job
	auto worker_1 = std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}});
	auto request_1 = std::make_shared<request>(request::headers_t{{"env", "c"}}, job_request_data("job_id", {}));
	worker_1->liveness = 1;
	worker_1->enqueue_request(request_1);
	ASSERT_TRUE(worker_1->next_request());
	workers->add_worker(worker_1);

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, nullptr);

	// We got a progress message from our worker
	handler.on_request(
		message_container(broker_connect::KEY_WORKERS, worker_1->identity, {"progress", "arg1", "arg2"}), respond);

	// We should just forward it to the monitor
	ASSERT_THAT(messages, ElementsAre(message_container(broker_connect::KEY_MONITOR, "", {"arg1", "arg2"})));
}

TEST(broker, worker_job_failed)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();

	// There is already a worker in the registry and it has a job
	auto worker_1 = std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}});
	auto request_1 = std::make_shared<request>(request::headers_t{{"env", "c"}}, job_request_data("job_id", {}));
	worker_1->liveness = 1;
	worker_1->enqueue_request(request_1);
	ASSERT_TRUE(worker_1->next_request());
	workers->add_worker(worker_1);

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, nullptr);

	// We got a message from our worker that says evaluation failed
	handler.on_request(
		message_container(
			broker_connect::KEY_WORKERS, worker_1->identity, {"done", "job_id", "ERR", "Testing failure"}),
		respond);

	// We should notify the frontend
	ASSERT_THAT(messages,
		ElementsAre(message_container(broker_connect::KEY_STATUS_NOTIFIER,
			"",
			{"type", "job_status", "id", "job_id", "status", "FAILED", "message", "Testing failure"})));

	messages.clear();
}

TEST(broker, worker_job_done)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto workers = std::make_shared<worker_registry>();

	// There is already a worker in the registry and it has a job
	auto worker_1 = std::make_shared<worker>("identity_1", "group_1", worker_headers_t{{"env", "c"}});
	auto request_1 = std::make_shared<request>(request::headers_t{{"env", "c"}}, job_request_data("job_id", {}));
	worker_1->liveness = 1;
	worker_1->enqueue_request(request_1);
	ASSERT_TRUE(worker_1->next_request());
	workers->add_worker(worker_1);

	// Dummy response callback
	std::vector<message_container> messages;
	handler_interface::response_cb respond = [&messages](const message_container &msg) { messages.push_back(msg); };

	// The test code
	broker_handler handler(config, workers, nullptr);

	// We got a message from our worker that says evaluation is done
	handler.on_request(
		message_container(broker_connect::KEY_WORKERS, worker_1->identity, {"done", "job_id", "OK", ""}), respond);

	// We should notify the frontend
	ASSERT_THAT(messages,
		ElementsAre(message_container(
			broker_connect::KEY_STATUS_NOTIFIER, "", {"type", "job_status", "id", "job_id", "status", "OK"})));

	messages.clear();
}
