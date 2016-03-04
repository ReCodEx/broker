#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../src/broker_connect.h"

using namespace testing;

class mock_broker_config : public broker_config {
public:
	MOCK_CONST_METHOD0(get_client_address, const std::string &());
	MOCK_CONST_METHOD0(get_client_port, uint16_t());
	MOCK_CONST_METHOD0(get_worker_address, const std::string &());
	MOCK_CONST_METHOD0(get_worker_port, uint16_t());
};

class mock_worker_registry : public worker_registry {
public:
	MOCK_METHOD1(add_worker, void(worker_registry::worker_ptr));
	MOCK_METHOD1(deprioritize_worker, void(worker_registry::worker_ptr));
	MOCK_METHOD1(find_worker, worker_registry::worker_ptr(const worker_registry::headers_t &));
	MOCK_METHOD1(find_worker_by_identity, worker_registry::worker_ptr(const std::string &));
};

class mock_connection_proxy {
public:
	MOCK_METHOD2(bind, void(const std::string &, const std::string &));
	MOCK_METHOD4(poll, void(message_origin::set &, std::chrono::milliseconds, bool &, std::chrono::milliseconds &));
	MOCK_METHOD3(recv_workers, bool(std::string &, std::vector<std::string> &, bool *));
	MOCK_METHOD3(recv_clients, bool(std::string &, std::vector<std::string> &, bool *));
	MOCK_METHOD2(send_workers, bool(const std::string &, const std::vector<std::string> &));
	MOCK_METHOD2(send_clients, bool(const std::string &, const std::vector<std::string> &));
};

TEST(broker, bind)
{
	auto config = std::make_shared<mock_broker_config>();
	auto sockets = std::make_shared<mock_connection_proxy>();
	auto workers = std::make_shared<mock_worker_registry>();
	const std::string address = "*";

	EXPECT_CALL(*config, get_client_address())
		.WillRepeatedly(ReturnRef(address));

	EXPECT_CALL(*config, get_client_port())
		.WillRepeatedly(Return(1234));

	EXPECT_CALL(*config, get_worker_address())
		.WillRepeatedly(ReturnRef(address));

	EXPECT_CALL(*config, get_worker_port())
		.WillRepeatedly(Return(4321));

	{
		InSequence s;

		std::string addr_1 = "tcp://*:1234";
		std::string addr_2 = "tcp://*:4321";

		EXPECT_CALL(*sockets, bind(StrEq(addr_1), StrEq(addr_2)));
		EXPECT_CALL(*sockets, poll(_, _, _, _)).WillOnce(SetArgReferee<2>(true));
	}

	broker_connect<mock_connection_proxy> broker(config, sockets, workers);
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
	const std::string address = "*";

	Sequence s1, s2;

	EXPECT_CALL(*config, get_client_address())
		.WillRepeatedly(ReturnRef(address));

	EXPECT_CALL(*config, get_client_port())
		.WillRepeatedly(Return(1234));

	EXPECT_CALL(*config, get_worker_address())
		.WillRepeatedly(ReturnRef(address));

	EXPECT_CALL(*config, get_worker_port())
		.WillRepeatedly(Return(4321));

	EXPECT_CALL(*sockets, bind(_, _))
		.InSequence(s1, s2);

	EXPECT_CALL(*sockets, poll(_, _, _, _))
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

	worker_registry::headers_t headers_1{
			std::make_pair("env", "c"),
			std::make_pair("hwgroup", "group_1")
	};

	auto worker_1 = std::make_shared<worker>("identity1", headers_1);
	worker_1->liveness = 100;

	EXPECT_CALL(*workers, find_worker_by_identity(StrEq("identity1")))
		.Times(AnyNumber())
		.InSequence(s2)
		.WillRepeatedly(Return(nullptr));

	EXPECT_CALL(*workers, add_worker(AllOf(
		Pointee(Field(&worker::identity, StrEq(worker_1->identity))),
		Pointee(Field(&worker::headers, worker_1->headers)))))
		.InSequence(s2);

	EXPECT_CALL(*workers, find_worker_by_identity(StrEq(worker_1->identity)))
		.Times(AnyNumber())
		.InSequence(s2)
		.WillRepeatedly(Return(worker_1));

	EXPECT_CALL(*sockets, poll(_, _, _, _))
		.InSequence(s1)
		.WillOnce(SetArgReferee<2>(true));

	broker_connect<mock_connection_proxy> broker(config, sockets, workers);
	broker.start_brokering();
}

TEST(broker, queuing)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto sockets = std::make_shared<StrictMock<mock_connection_proxy>>();
	auto workers = std::make_shared<StrictMock<mock_worker_registry>>();
	const std::string address = "*";

	std::string client_id = "client_foo";
	worker_registry::headers_t headers = {{"env", "c"}};
	worker_registry::worker_ptr worker_1 = std::make_shared<worker>("identity1", headers);

	EXPECT_CALL(*config, get_client_address())
		.WillRepeatedly(ReturnRef(address));

	EXPECT_CALL(*config, get_client_port())
		.WillRepeatedly(Return(1234));

	EXPECT_CALL(*config, get_worker_address())
		.WillRepeatedly(ReturnRef(address));

	EXPECT_CALL(*config, get_worker_port())
		.WillRepeatedly(Return(4321));

	EXPECT_CALL(*workers, find_worker(Eq(headers)))
		.WillRepeatedly(Return(worker_1));

	EXPECT_CALL(*workers, find_worker_by_identity(worker_1->identity))
		.WillRepeatedly(Return(worker_1));

	Sequence s1, s2, s3;

	EXPECT_CALL(*sockets, bind(_, _))
		.InSequence(s1, s2);

	// A request has arrived
	EXPECT_CALL(*sockets, poll(_, _, _, _))
		.InSequence(s1)
		.WillOnce(DoAll(
			ClearFlags(), SetFlag(message_origin::CLIENT)
		));

	EXPECT_CALL(*sockets, recv_clients(_, _, _))
		.InSequence(s2, s3)
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

	EXPECT_CALL(*workers, deprioritize_worker(worker_1))
		.InSequence(s3);

	// Let the worker process it
	EXPECT_CALL(*sockets, send_workers(StrEq(worker_1->identity), ElementsAre("eval", "job1", "1", "2")))
		.InSequence(s2);

	EXPECT_CALL(*sockets, send_clients(StrEq(client_id), ElementsAre("accept")))
		.InSequence(s2);

	// Another request has arrived
	EXPECT_CALL(*sockets, poll(_, _, _, _))
		.InSequence(s1)
		.WillOnce(DoAll(
			ClearFlags(), SetFlag(message_origin::CLIENT)
		));

	EXPECT_CALL(*sockets, recv_clients(_, _, _))
		.InSequence(s2, s3)
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

	EXPECT_CALL(*workers, deprioritize_worker(worker_1))
		.InSequence(s3);

	// The worker is busy, but we don't want to keep the client waiting
	EXPECT_CALL(*sockets, send_clients(StrEq(client_id), ElementsAre("accept")))
		.InSequence(s2);

	// The worker is finally done
	EXPECT_CALL(*sockets, poll(_, _, _, _))
		.InSequence(s1)
		.WillOnce(DoAll(
			ClearFlags(), SetFlag(message_origin::WORKER)
		));

	EXPECT_CALL(*sockets, recv_workers(_, _, _))
		.InSequence(s2)
		.WillOnce(DoAll(
			SetArgReferee<0>(worker_1->identity),
			SetArgReferee<1>(std::vector<std::string>{
				"done",
				"job1"
			})
		));

	// Give the worker some more work
	EXPECT_CALL(*sockets, send_workers(StrEq(worker_1->identity), ElementsAre("eval", "job2", "3", "4")))
		.InSequence(s2);

	EXPECT_CALL(*sockets, poll(_, _, _, _))
		.InSequence(s1)
		.WillOnce(SetArgReferee<2>(true));

	broker_connect<mock_connection_proxy> broker(config, sockets, workers);
	broker.start_brokering();
}

TEST(broker, ping_unknown_worker)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto sockets = std::make_shared<StrictMock<mock_connection_proxy>>();
	auto workers = std::make_shared<StrictMock<mock_worker_registry>>();
	const std::string address = "*";

	std::string client_id = "client_foo";
	worker_registry::headers_t headers = {{"env", "c"}, {"hwgroup", "group_1"}};
	worker_registry::worker_ptr worker_1 = std::make_shared<worker>("identity1", headers);

	EXPECT_CALL(*config, get_client_address())
		.WillRepeatedly(ReturnRef(address));

	EXPECT_CALL(*config, get_client_port())
		.WillRepeatedly(Return(1234));

	EXPECT_CALL(*config, get_worker_address())
		.WillRepeatedly(ReturnRef(address));

	EXPECT_CALL(*config, get_worker_port())
		.WillRepeatedly(Return(4321));

	Sequence s1, s2, s3;

	EXPECT_CALL(*sockets, bind(_, _))
		.InSequence(s1, s2);

	EXPECT_CALL(*sockets, poll(_, _, _, _))
		.InSequence(s1)
		.WillOnce(DoAll(
			ClearFlags(), SetFlag(message_origin::WORKER)
		));

	// A wild ping appeared
	EXPECT_CALL(*sockets, recv_workers(_, _, _))
		.InSequence(s2, s3)
		.WillOnce(DoAll(
			SetArgReferee<0>(worker_1->identity),
			SetArgReferee<1>(std::vector<std::string>{
				"ping"
			})
		));

	EXPECT_CALL(*workers, find_worker_by_identity(StrEq(worker_1->identity)))
		.InSequence(s3)
		.WillRepeatedly(Return(nullptr));

	// Ask the worker to introduce itself
	EXPECT_CALL(*sockets, send_workers(StrEq(worker_1->identity), ElementsAre("intro")))
		.InSequence(s2);

	// The worker kindly does so
	EXPECT_CALL(*sockets, poll(_, _, _, _))
		.InSequence(s1)
		.WillOnce(DoAll(
			ClearFlags(), SetFlag(message_origin::WORKER)
		));

	EXPECT_CALL(*sockets, recv_workers(_, _, _))
		.InSequence(s2)
		.WillOnce(DoAll(
			SetArgReferee<0>(worker_1->identity),
			SetArgReferee<1>(std::vector<std::string>{
				"init",
				"env=c",
				"hwgroup=group_1"
			})
		));

	EXPECT_CALL(*workers, add_worker(AllOf(
		Pointee(Field(&worker::identity, StrEq(worker_1->identity))),
		Pointee(Field(&worker::headers, Eq(worker_1->headers)))
	))).InSequence(s2);

	// Last poll
	EXPECT_CALL(*sockets, poll(_, _, _, _))
		.InSequence(s1)
		.WillOnce(SetArgReferee<2>(true));

	broker_connect<mock_connection_proxy> broker(config, sockets, workers);
	broker.start_brokering();
}

TEST(broker, ping_known_worker)
{
	auto config = std::make_shared<NiceMock<mock_broker_config>>();
	auto sockets = std::make_shared<StrictMock<mock_connection_proxy>>();
	auto workers = std::make_shared<StrictMock<mock_worker_registry>>();
	const std::string address = "*";

	std::string client_id = "client_foo";
	worker_registry::headers_t headers = {{"env", "c"}, {"hwgroup", "group_1"}};
	worker_registry::worker_ptr worker_1 = std::make_shared<worker>("identity1", headers);

	EXPECT_CALL(*config, get_client_address())
		.WillRepeatedly(ReturnRef(address));

	EXPECT_CALL(*config, get_client_port())
		.WillRepeatedly(Return(1234));

	EXPECT_CALL(*config, get_worker_address())
		.WillRepeatedly(ReturnRef(address));

	EXPECT_CALL(*config, get_worker_port())
		.WillRepeatedly(Return(4321));

	Sequence s1, s2, s3;

	EXPECT_CALL(*sockets, bind(_, _))
		.InSequence(s1, s2);

	EXPECT_CALL(*sockets, poll(_, _, _, _))
		.InSequence(s1)
		.WillOnce(DoAll(
			ClearFlags(), SetFlag(message_origin::WORKER)
		));

	// A ping from a familiar worker appeared
	EXPECT_CALL(*sockets, recv_workers(_, _, _))
		.InSequence(s2, s3)
		.WillOnce(DoAll(
			SetArgReferee<0>(worker_1->identity),
			SetArgReferee<1>(std::vector<std::string>{
				"ping"
			})
		));

	EXPECT_CALL(*workers, find_worker_by_identity(StrEq(worker_1->identity)))
		.InSequence(s3)
		.WillRepeatedly(Return(worker_1));

	// Ask the worker to introduce itself
	EXPECT_CALL(*sockets, send_workers(StrEq(worker_1->identity), ElementsAre("pong")))
		.InSequence(s2);

	// Last poll
	EXPECT_CALL(*sockets, poll(_, _, _, _))
		.InSequence(s1)
		.WillOnce(SetArgReferee<2>(true));

	broker_connect<mock_connection_proxy> broker(config, sockets, workers);
	broker.start_brokering();
}