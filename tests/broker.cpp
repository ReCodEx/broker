#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "mocks.h"

using namespace testing;


/** Check if worker satisfies given header value */
MATCHER_P2(HasHeader, header, value, std::string("Worker satisfies header ") + header + "=" + value)
{
	return ((std::shared_ptr<worker>) arg)->check_header(header, value);
}

TEST(broker, bind)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto sockets = std::make_shared<mock_connection_proxy>();
	auto workers = std::make_shared<mock_worker_registry>();

	{
		InSequence s;

		std::string addr_1 = "tcp://*:1234";
		std::string addr_2 = "tcp://*:4321";
		std::string addr_3 = "tcp://127.0.0.1:7894";

		EXPECT_CALL(*sockets, set_addresses(StrEq(addr_1), StrEq(addr_2), StrEq(addr_3)));
		EXPECT_CALL(*sockets, poll(_, _, _, _)).WillOnce(SetArgReferee<2>(true));
	}

	broker_connect<mock_connection_proxy> broker(config, sockets, workers, nullptr);
	broker.start_brokering();
}

ACTION(ClearFlags)
{
	((message_origin::set &) arg0).reset();
}

ACTION_P(SetFlag, flag)
{
	((message_origin::set &) arg0).set(flag, true);
}

TEST(broker, worker_init)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto sockets = std::make_shared<StrictMock<mock_connection_proxy>>();
	auto workers = std::make_shared<StrictMock<mock_worker_registry>>();

	Sequence s1, s2, s3, s4;

	EXPECT_CALL(*sockets, set_addresses(_, _, _)).InSequence(s1, s2);

	EXPECT_CALL(*sockets, poll(_, _, _, _))
		.InSequence(s1)
		.WillOnce(DoAll(ClearFlags(), SetFlag(message_origin::WORKER)));

	// A wild worker appeared
	EXPECT_CALL(*sockets, recv_workers(_, _, _))
		.InSequence(s2, s3)
		.WillOnce(DoAll(SetArgReferee<0>("identity1"),
			SetArgReferee<1>(std::vector<std::string>{"init", "group_1", "env=c", "threads=8"})));

	request::headers_t headers_1{std::make_pair("env", "c"), std::make_pair("threads", "8")};

	auto worker_1 = std::make_shared<worker>("identity1", "group_1", headers_1);
	worker_1->liveness = 100;

	std::vector<std::shared_ptr<worker>> empty_vector;
	std::vector<std::shared_ptr<worker>> nonempty_vector = {worker_1};

	EXPECT_CALL(*workers, find_worker_by_identity(StrEq("identity1")))
		.Times(AnyNumber())
		.InSequence(s2)
		.WillRepeatedly(Return(nullptr));

	EXPECT_CALL(*workers, get_workers()).Times(AnyNumber()).InSequence(s3).WillRepeatedly(ReturnRef(empty_vector));

	EXPECT_CALL(*workers,
		add_worker(AllOf(Pointee(Field(&worker::identity, StrEq(worker_1->identity))),
			HasHeader("env", "c"),
			HasHeader("threads", "8"),
			HasHeader("hwgroup", worker_1->hwgroup),
			Pointee(Field(&worker::hwgroup, worker_1->hwgroup)))))
		.InSequence(s2, s4);

	EXPECT_CALL(*workers, find_worker_by_identity(StrEq(worker_1->identity)))
		.Times(AnyNumber())
		.InSequence(s2)
		.WillRepeatedly(Return(worker_1));

	EXPECT_CALL(*workers, get_workers()).Times(AnyNumber()).InSequence(s4).WillRepeatedly(ReturnRef(nonempty_vector));

	EXPECT_CALL(*sockets, poll(_, _, _, _)).InSequence(s1).WillOnce(SetArgReferee<2>(true));

	broker_connect<mock_connection_proxy> broker(config, sockets, workers, nullptr);
	broker.start_brokering();
}

TEST(broker, queuing)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto sockets = std::make_shared<StrictMock<mock_connection_proxy>>();
	auto workers = std::make_shared<StrictMock<mock_worker_registry>>();
	const std::string address = "*";

	std::string client_id = "client_foo";
	request::headers_t headers = {{"env", "c"}};
	worker_registry::worker_ptr worker_1 = std::make_shared<worker>("identity1", "group_1", headers);
	worker_1->liveness = 100;

	std::vector<std::shared_ptr<worker>> worker_vector = {worker_1};

	EXPECT_CALL(*config, get_worker_ping_interval()).WillRepeatedly(Return(std::chrono::milliseconds(50000)));

	EXPECT_CALL(*workers, find_worker(Eq(headers))).WillRepeatedly(Return(worker_1));

	EXPECT_CALL(*workers, find_worker_by_identity(worker_1->identity)).WillRepeatedly(Return(worker_1));

	EXPECT_CALL(*workers, get_workers()).WillRepeatedly(ReturnRef(worker_vector));

	EXPECT_CALL(*workers, remove_worker(_)).Times(0);

	Sequence s1, s2, s3;

	EXPECT_CALL(*sockets, set_addresses(_, _, _)).InSequence(s1, s2);

	// A request has arrived
	EXPECT_CALL(*sockets, poll(_, _, _, _))
		.InSequence(s1)
		.WillOnce(
			DoAll(ClearFlags(), SetFlag(message_origin::CLIENT), SetArgReferee<3>(std::chrono::milliseconds(10))));

	EXPECT_CALL(*sockets, recv_clients(_, _, _))
		.InSequence(s2, s3)
		.WillOnce(DoAll(SetArgReferee<0>(client_id),
			SetArgReferee<1>(std::vector<std::string>{"eval", "job1", "env=c", "", "1", "2"})));

	EXPECT_CALL(*workers, deprioritize_worker(worker_1)).InSequence(s3);

	// Let the worker process it
	EXPECT_CALL(*sockets, send_workers(StrEq(worker_1->identity), ElementsAre("eval", "job1", "1", "2")))
		.InSequence(s2);

	EXPECT_CALL(*sockets, send_clients(StrEq(client_id), ElementsAre("accept"))).InSequence(s2);

	// Another request has arrived
	EXPECT_CALL(*sockets, poll(_, _, _, _))
		.InSequence(s1)
		.WillOnce(
			DoAll(ClearFlags(), SetFlag(message_origin::CLIENT), SetArgReferee<3>(std::chrono::milliseconds(10))));

	EXPECT_CALL(*sockets, recv_clients(_, _, _))
		.InSequence(s2, s3)
		.WillOnce(DoAll(SetArgReferee<0>(client_id),
			SetArgReferee<1>(std::vector<std::string>{"eval", "job2", "env=c", "", "3", "4"})));

	EXPECT_CALL(*workers, deprioritize_worker(worker_1)).InSequence(s3);

	// The worker is busy, but we don't want to keep the client waiting
	EXPECT_CALL(*sockets, send_clients(StrEq(client_id), ElementsAre("accept"))).InSequence(s2);

	// The worker is finally done
	EXPECT_CALL(*sockets, poll(_, _, _, _))
		.InSequence(s1)
		.WillOnce(
			DoAll(ClearFlags(), SetFlag(message_origin::WORKER), SetArgReferee<3>(std::chrono::milliseconds(10))));

	EXPECT_CALL(*sockets, recv_workers(_, _, _))
		.InSequence(s2)
		.WillOnce(
			DoAll(SetArgReferee<0>(worker_1->identity), SetArgReferee<1>(std::vector<std::string>{"done", "job1"})));

	// Give the worker some more work
	EXPECT_CALL(*sockets, send_workers(StrEq(worker_1->identity), ElementsAre("eval", "job2", "3", "4")))
		.InSequence(s2);

	// Last poll
	EXPECT_CALL(*sockets, poll(_, _, _, _)).InSequence(s1).WillOnce(SetArgReferee<2>(true));

	broker_connect<mock_connection_proxy> broker(config, sockets, workers, nullptr);
	broker.start_brokering();
}

TEST(broker, ping_unknown_worker)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto sockets = std::make_shared<StrictMock<mock_connection_proxy>>();
	auto workers = std::make_shared<StrictMock<mock_worker_registry>>();
	const std::string address = "*";

	request::headers_t headers = {{"env", "c"}};
	worker_registry::worker_ptr worker_1 = std::make_shared<worker>("identity1", "group_1", headers);
	worker_1->liveness = 100;

	std::vector<std::shared_ptr<worker>> empty_worker_vector;
	std::vector<std::shared_ptr<worker>> worker_vector = {worker_1};

	Sequence s1, s2, s3, s4, s5;

	EXPECT_CALL(*sockets, set_addresses(_, _, _)).InSequence(s1, s2);

	EXPECT_CALL(*sockets, poll(_, _, _, _))
		.InSequence(s1)
		.WillOnce(DoAll(ClearFlags(), SetFlag(message_origin::WORKER)));

	// A wild ping appeared
	EXPECT_CALL(*sockets, recv_workers(_, _, _))
		.InSequence(s2, s3, s5)
		.WillOnce(DoAll(SetArgReferee<0>(worker_1->identity), SetArgReferee<1>(std::vector<std::string>{"ping"})));

	EXPECT_CALL(*workers, find_worker_by_identity(StrEq(worker_1->identity)))
		.InSequence(s3)
		.WillRepeatedly(Return(nullptr));

	EXPECT_CALL(*workers, get_workers()).Times(AnyNumber()).WillRepeatedly(ReturnRef(empty_worker_vector));

	// Ask the worker to introduce itself
	EXPECT_CALL(*sockets, send_workers(StrEq(worker_1->identity), ElementsAre("intro"))).InSequence(s2);

	// The worker kindly does so
	EXPECT_CALL(*sockets, poll(_, _, _, _))
		.InSequence(s1)
		.WillOnce(DoAll(ClearFlags(), SetFlag(message_origin::WORKER)));

	EXPECT_CALL(*sockets, recv_workers(_, _, _))
		.InSequence(s2)
		.WillOnce(DoAll(SetArgReferee<0>(worker_1->identity),
			SetArgReferee<1>(std::vector<std::string>{"init", "group_1", "env=c"})));

	EXPECT_CALL(*workers,
		add_worker(AllOf(Pointee(Field(&worker::identity, StrEq(worker_1->identity))),
			Pointee(Field(&worker::hwgroup, StrEq(worker_1->hwgroup))),
			HasHeader("env", "c"),
			HasHeader("hwgroup", "group_1"))))
		.InSequence(s2, s4);

	EXPECT_CALL(*workers, get_workers()).Times(AnyNumber()).InSequence(s4).WillRepeatedly(ReturnRef(worker_vector));

	// Last poll
	EXPECT_CALL(*sockets, poll(_, _, _, _)).InSequence(s1).WillOnce(SetArgReferee<2>(true));

	broker_connect<mock_connection_proxy> broker(config, sockets, workers, nullptr);
	broker.start_brokering();
}

TEST(broker, ping_known_worker)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto sockets = std::make_shared<StrictMock<mock_connection_proxy>>();
	auto workers = std::make_shared<StrictMock<mock_worker_registry>>();

	request::headers_t headers = {{"env", "c"}};
	worker_registry::worker_ptr worker_1 = std::make_shared<worker>("identity1", "group_1", headers);
	worker_1->liveness = 100;

	std::vector<std::shared_ptr<worker>> worker_vector = {worker_1};

	EXPECT_CALL(*workers, find_worker_by_identity(StrEq(worker_1->identity))).WillRepeatedly(Return(worker_1));

	EXPECT_CALL(*workers, get_workers()).WillRepeatedly(ReturnRef(worker_vector));

	Sequence s1, s2, s3;

	EXPECT_CALL(*sockets, set_addresses(_, _, _)).InSequence(s1, s2);

	EXPECT_CALL(*sockets, poll(_, _, _, _))
		.InSequence(s1)
		.WillOnce(DoAll(ClearFlags(), SetFlag(message_origin::WORKER)));

	// A ping from a familiar worker appeared
	EXPECT_CALL(*sockets, recv_workers(_, _, _))
		.InSequence(s2, s3)
		.WillOnce(DoAll(SetArgReferee<0>(worker_1->identity), SetArgReferee<1>(std::vector<std::string>{"ping"})));

	// Respond to the ping
	EXPECT_CALL(*sockets, send_workers(StrEq(worker_1->identity), ElementsAre("pong"))).InSequence(s2);

	// Last poll
	EXPECT_CALL(*sockets, poll(_, _, _, _)).InSequence(s1).WillOnce(SetArgReferee<2>(true));

	broker_connect<mock_connection_proxy> broker(config, sockets, workers, nullptr);
	broker.start_brokering();
}

TEST(broker, worker_expiration)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto sockets = std::make_shared<StrictMock<mock_connection_proxy>>();
	auto workers = std::make_shared<StrictMock<mock_worker_registry>>();
	auto notifier = std::make_shared<mock_status_notifier>();

	request::headers_t headers = {{"env", "c"}};
	auto worker_1 = std::make_shared<mock_worker>("identity1", "group_1", headers);
	worker_1->liveness = 1;

	// prepare request list which will be returned on worker termination
	job_request_data request_data("job_id", {});
	auto req = std::make_shared<request>(headers, request_data);
	auto reqs = std::make_shared<std::vector<worker::request_ptr>>();
	reqs->push_back(req);

	std::vector<std::shared_ptr<worker>> worker_vector{worker_1};

	Sequence s1, s2;

	EXPECT_CALL(*sockets, set_addresses(_, _, _)).InSequence(s1, s2);

	// No message from workers for a whole second
	EXPECT_CALL(*sockets, poll(_, _, _, _))
		.InSequence(s1)
		.WillOnce(DoAll(ClearFlags(), SetArgReferee<3>(std::chrono::milliseconds(1000))));

	EXPECT_CALL(*workers, get_workers()).WillRepeatedly(ReturnRef(worker_vector));

	// Liveness got decreased to zero -> get rid of the worker
	EXPECT_CALL(*workers, remove_worker(Eq(worker_1))).InSequence(s2);

	// On worker termination return list of active requests
	EXPECT_CALL(*worker_1, terminate()).InSequence(s2).WillOnce(Return(reqs));

	// No substitute workers are available for reassignment
	EXPECT_CALL(*workers, find_worker(_)).InSequence(s2).WillRepeatedly(Return(nullptr));

	// Worker was terminated but had some jobs which cannot be assigned to different worker
	// frontend has to be notified through notifier
	EXPECT_CALL(*notifier, rejected_jobs(_, _)).InSequence(s2);

	// Last poll
	EXPECT_CALL(*sockets, poll(_, _, _, _)).InSequence(s1).WillOnce(SetArgReferee<2>(true));

	broker_connect<mock_connection_proxy> broker(config, sockets, workers, notifier);
	broker.start_brokering();

	// Cleanup
	Mock::VerifyAndClearExpectations(worker_1.get());
}

TEST(broker, worker_state_message)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto sockets = std::make_shared<StrictMock<mock_connection_proxy>>();
	auto workers = std::make_shared<StrictMock<mock_worker_registry>>();

	request::headers_t headers = {{"env", "c"}};
	worker_registry::worker_ptr worker_1 = std::make_shared<worker>("identity1", "group_1", headers);
	worker_1->liveness = 100;

	std::vector<std::shared_ptr<worker>> worker_vector = {worker_1};

	EXPECT_CALL(*workers, find_worker_by_identity(StrEq(worker_1->identity))).WillRepeatedly(Return(worker_1));

	EXPECT_CALL(*workers, get_workers()).WillRepeatedly(ReturnRef(worker_vector));

	Sequence s1, s2, s3;

	EXPECT_CALL(*sockets, set_addresses(_, _, _)).InSequence(s1, s2);

	EXPECT_CALL(*sockets, poll(_, _, _, _))
		.InSequence(s1)
		.WillOnce(DoAll(ClearFlags(), SetFlag(message_origin::WORKER)));

	// A state command from worker
	EXPECT_CALL(*sockets, recv_workers(_, _, _))
		.InSequence(s2, s3)
		.WillOnce(DoAll(SetArgReferee<0>(worker_1->identity),
			SetArgReferee<1>(std::vector<std::string>{"progress", "arg1", "arg2"})));

	// Respond to command - forward arguments to the monitor
	EXPECT_CALL(*sockets, send_monitor(ElementsAre("arg1", "arg2"))).InSequence(s2);

	// Last poll
	EXPECT_CALL(*sockets, poll(_, _, _, _)).InSequence(s1).WillOnce(SetArgReferee<2>(true));

	broker_connect<mock_connection_proxy> broker(config, sockets, workers, nullptr);
	broker.start_brokering();
}

TEST(broker, worker_job_failed)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto sockets = std::make_shared<StrictMock<mock_connection_proxy>>();
	auto workers = std::make_shared<StrictMock<mock_worker_registry>>();
	auto notifier = std::make_shared<mock_status_notifier>();

	request::headers_t headers = {{"env", "c"}};
	auto worker_1 = std::make_shared<mock_worker>("identity1", "group_1", headers);
	worker_1->liveness = 1;

	// prepare request list which will be returned on worker termination
	std::string job_id = "job_id";
	job_request_data request_data(job_id, {});
	auto req = std::make_shared<request>(headers, request_data);
	std::vector<std::string> done_message = {"done", job_id, "ERR", "Testing failure"};

	std::vector<std::shared_ptr<worker>> worker_vector{worker_1};
	EXPECT_CALL(*workers, get_workers()).WillRepeatedly(ReturnRef(worker_vector));

	{
		InSequence s1;

		EXPECT_CALL(*sockets, set_addresses(_, _, _));

		EXPECT_CALL(*sockets, poll(_, _, _, _)).WillOnce(DoAll(ClearFlags(), SetFlag(message_origin::WORKER)));

		// A job done message appeared from worker
		EXPECT_CALL(*sockets, recv_workers(_, _, _))
			.WillOnce(DoAll(SetArgReferee<0>(worker_1->identity), SetArgReferee<1>(done_message)));

		// find worker after job done message was received
		EXPECT_CALL(*workers, find_worker_by_identity(_)).WillOnce(Return(worker_1));

		// get current worker request
		EXPECT_CALL(*worker_1, get_current_request()).WillOnce(Return(req));

		// notifier should notify frontend about job failure
		EXPECT_CALL(*notifier, job_failed(StrEq(job_id), StrEq(done_message.at(3))));

		EXPECT_CALL(*worker_1, complete_request());

		EXPECT_CALL(*worker_1, next_request()).WillOnce(Return(false));

		// worker liveness
		EXPECT_CALL(*workers, find_worker_by_identity(_)).WillOnce(Return(worker_1));

		// Last poll
		EXPECT_CALL(*sockets, poll(_, _, _, _)).WillOnce(SetArgReferee<2>(true));
	}

	broker_connect<mock_connection_proxy> broker(config, sockets, workers, notifier);
	broker.start_brokering();

	// Cleanup
	Mock::VerifyAndClearExpectations(worker_1.get());
}

TEST(broker, worker_job_done)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto sockets = std::make_shared<StrictMock<mock_connection_proxy>>();
	auto workers = std::make_shared<StrictMock<mock_worker_registry>>();
	auto notifier = std::make_shared<mock_status_notifier>();

	request::headers_t headers = {{"env", "c"}};
	auto worker_1 = std::make_shared<mock_worker>("identity1", "group_1", headers);
	worker_1->liveness = 1;

	// prepare request list which will be returned on worker termination
	std::string job_id = "job_id";
	job_request_data request_data(job_id, {});
	auto req = std::make_shared<request>(headers, request_data);
	std::vector<std::string> done_message = {"done", job_id, "OK", ""};

	std::vector<std::shared_ptr<worker>> worker_vector{worker_1};
	EXPECT_CALL(*workers, get_workers()).WillRepeatedly(ReturnRef(worker_vector));

	{
		InSequence s1;

		EXPECT_CALL(*sockets, set_addresses(_, _, _));

		EXPECT_CALL(*sockets, poll(_, _, _, _)).WillOnce(DoAll(ClearFlags(), SetFlag(message_origin::WORKER)));

		// A job done message appeared from worker
		EXPECT_CALL(*sockets, recv_workers(_, _, _))
			.WillOnce(DoAll(SetArgReferee<0>(worker_1->identity), SetArgReferee<1>(done_message)));

		// find worker after job done message was received
		EXPECT_CALL(*workers, find_worker_by_identity(_)).WillOnce(Return(worker_1));

		// get current worker request
		EXPECT_CALL(*worker_1, get_current_request()).WillOnce(Return(req));

		// notifier should notify frontend about job done
		EXPECT_CALL(*notifier, job_done(StrEq(job_id)));

		EXPECT_CALL(*worker_1, complete_request());

		EXPECT_CALL(*worker_1, next_request()).WillOnce(Return(false));

		// worker liveness
		EXPECT_CALL(*workers, find_worker_by_identity(_)).WillOnce(Return(worker_1));

		// Last poll
		EXPECT_CALL(*sockets, poll(_, _, _, _)).WillOnce(SetArgReferee<2>(true));
	}

	broker_connect<mock_connection_proxy> broker(config, sockets, workers, notifier);
	broker.start_brokering();

	// Cleanup
	Mock::VerifyAndClearExpectations(worker_1.get());
}
