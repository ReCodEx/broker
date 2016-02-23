#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../src/broker_connect.h"

using namespace testing;

class mock_broker_config : public broker_config {
public:
	MOCK_CONST_METHOD0(get_client_port, uint16_t());
	MOCK_CONST_METHOD0(get_worker_port, uint16_t());
};

class mock_task_router : public task_router {
public:
	MOCK_METHOD1(add_worker, void(task_router::worker_ptr));
	MOCK_METHOD1(find_worker, task_router::worker_ptr(const task_router::headers_t &));
	MOCK_METHOD1(find_worker_by_identity, task_router::worker_ptr(const std::string &));
};

class mock_connection_proxy {
public:
	MOCK_METHOD2(bind, void(const std::string &, const std::string &));
	MOCK_METHOD3(poll, void(message_origin::set &, int, bool *));
	MOCK_METHOD3(recv_workers, bool(std::string &, std::vector<std::string> &, bool *));
	MOCK_METHOD3(recv_clients, bool(std::string &, std::vector<std::string> &, bool *));
	MOCK_METHOD2(send_workers, bool(const std::string &, const std::vector<std::string> &));
	MOCK_METHOD2(send_clients, bool(const std::string &, const std::vector<std::string> &));
};

TEST(broker, bind)
{
	auto config = std::make_shared<mock_broker_config>();
	auto sockets = std::make_shared<mock_connection_proxy>();
	auto router = std::make_shared<mock_task_router>();

	EXPECT_CALL(*config, get_client_port())
		.WillRepeatedly(Return(1234));

	EXPECT_CALL(*config, get_worker_port())
		.WillRepeatedly(Return(4321));

	{
		InSequence s;

		std::string addr_1 = "tcp://*:1234";
		std::string addr_2 = "tcp://*:4321";

		EXPECT_CALL(*sockets, bind(StrEq(addr_1), StrEq(addr_2)));
		EXPECT_CALL(*sockets, poll(_, _, _)).WillOnce(SetArgPointee<2>(true));
	}

	broker_connect<mock_connection_proxy> broker(config, sockets, router);
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
	auto router = std::make_shared<StrictMock<mock_task_router>>();

	Sequence s1, s2;

	EXPECT_CALL(*sockets, bind(_, _))
		.InSequence(s1, s2);

	EXPECT_CALL(*sockets, poll(_, _, _))
		.InSequence(s1)
		.WillOnce(DoAll(
			ClearFlags(), SetFlag(message_origin::WORKER)
		));

	// A wild worker appeared
	EXPECT_CALL(*sockets, recv_workers(_, _, _))
		.InSequence(s2)
		.WillOnce(DoAll(
			SetArgReferee<0>("identity1"),
			SetArgReferee<1>(std::vector<std::string>{
				"init",
				"env=c",
				"hwgroup=group_1"
			})
		));

	EXPECT_CALL(*router, add_worker(AllOf(
		Pointee(Field(&worker::identity, StrEq("identity1"))),
		Pointee(Field(&worker::headers, UnorderedElementsAre(
			std::pair<std::string, std::string>{"env", "c"},
			std::pair<std::string, std::string>{"hwgroup", "group_1"}
		))))))
		.InSequence(s2);

	EXPECT_CALL(*sockets, poll(_, _, _))
		.InSequence(s1)
		.WillOnce(SetArgPointee<2>(true));

	broker_connect<mock_connection_proxy> broker(config, sockets, router);
	broker.start_brokering();
}

TEST(broker, queuing)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto sockets = std::make_shared<StrictMock<mock_connection_proxy>>();
	auto router = std::make_shared<StrictMock<mock_task_router>>();

	std::string client_id = "client_foo";
	task_router::headers_t headers = {{"env", "c"}};
	task_router::worker_ptr worker = std::make_shared<struct worker>("identity1", headers);

	EXPECT_CALL(*router, find_worker(Eq(headers)))
		.WillRepeatedly(Return(worker));

	EXPECT_CALL(*router, find_worker_by_identity(worker->identity))
		.WillRepeatedly(Return(worker));

	Sequence s1, s2;

	EXPECT_CALL(*sockets, bind(_, _))
		.InSequence(s1, s2);

	// A request has arrived
	EXPECT_CALL(*sockets, poll(_, _, _))
		.InSequence(s1)
		.WillOnce(DoAll(
			ClearFlags(), SetFlag(message_origin::CLIENT)
		));

	EXPECT_CALL(*sockets, recv_clients(_, _, _))
		.InSequence(s2)
		.WillOnce(DoAll(
			SetArgReferee<0>(client_id),
			SetArgReferee<1>(std::vector<std::string>{
				"eval",
				"job1",
				"env=c",
				"",
				"1",
				"2"
			})
		));

	// Let the worker process it
	EXPECT_CALL(*sockets, send_workers(StrEq(worker->identity), ElementsAre("eval", "job1", "1", "2")))
		.InSequence(s2);

	EXPECT_CALL(*sockets, send_clients(StrEq(client_id), ElementsAre("accept")))
		.InSequence(s2);

	// Another request has arrived
	EXPECT_CALL(*sockets, poll(_, _, _))
		.InSequence(s1)
		.WillOnce(DoAll(
			ClearFlags(), SetFlag(message_origin::CLIENT)
		));

	EXPECT_CALL(*sockets, recv_clients(_, _, _))
		.InSequence(s2)
		.WillOnce(DoAll(
			SetArgReferee<0>(client_id),
			SetArgReferee<1>(std::vector<std::string>{
				"eval",
				"job2",
				"env=c",
				"",
				"3",
				"4"
			})
		));

	// The worker is busy, but we don't want to keep the client waiting
	EXPECT_CALL(*sockets, send_clients(StrEq(client_id), ElementsAre("accept")))
		.InSequence(s2);

	// The worker is finally done
	EXPECT_CALL(*sockets, poll(_, _, _))
		.InSequence(s1)
		.WillOnce(DoAll(
			ClearFlags(), SetFlag(message_origin::WORKER)
		));

	EXPECT_CALL(*sockets, recv_workers(_, _, _))
		.InSequence(s2)
		.WillOnce(DoAll(
			SetArgReferee<0>(worker->identity),
			SetArgReferee<1>(std::vector<std::string>{
				"done",
				"job1"
			})
		));

	// Give the worker some more work
	EXPECT_CALL(*sockets, send_workers(worker->identity, ElementsAre("eval", "job2", "3", "4")))
		.InSequence(s2);

	EXPECT_CALL(*sockets, poll(_, _, _))
		.InSequence(s1)
		.WillOnce(SetArgPointee<2>(true));

	broker_connect<mock_connection_proxy> broker(config, sockets, router);
	broker.start_brokering();
}