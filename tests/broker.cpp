#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../src/broker.h"

using namespace testing;

class mock_broker_config : public broker_config {
public:
	MOCK_CONST_METHOD0(get_client_port, uint16_t());
	MOCK_CONST_METHOD0(get_worker_port, uint16_t());
};

class mock_connection_proxy {
public:
	MOCK_METHOD2(bind, void(const std::string &, const std::string &));
	MOCK_METHOD2(poll, void(message_receiver::set &, int));
	MOCK_METHOD2(recv_workers_id, void(zmq::message_t &, std::string &));
	MOCK_METHOD1(recv_workers, void(zmq::message_t &));
	MOCK_METHOD2(recv_clients_id, void(zmq::message_t &, std::string &));
	MOCK_METHOD1(recv_clients, void(zmq::message_t &));
	MOCK_METHOD1(send_workers_id, void(const std::string &));
	MOCK_METHOD3(send_workers, void(const void *, size_t, int));
	MOCK_METHOD1(send_clients_id, void(const std::string &));
	MOCK_METHOD3(send_clients, void(const void *, size_t, int));
};

TEST(broker, bind)
{
	mock_broker_config config;
	auto sockets = std::make_shared<mock_connection_proxy>();

	EXPECT_CALL(config, get_client_port())
		.WillRepeatedly(Return(1234));

	EXPECT_CALL(config, get_worker_port())
		.WillRepeatedly(Return(4321));

	{
		InSequence s;

		std::string addr_1 = "tcp://*:1234";
		std::string addr_2 = "tcp://*:4321";

		EXPECT_CALL(*sockets, bind(StrEq(addr_1), StrEq(addr_2)));
		EXPECT_CALL(*sockets, poll(_, _)).WillOnce(Throw(zmq::error_t()));
	}

	broker<mock_connection_proxy> broker(config, sockets);
	broker.start_brokering();
}

ACTION(ClearFlags)
{
	((message_receiver::set &) arg0).reset();
}

ACTION_P(SetFlag, flag)
{
	((message_receiver::set &) arg0).set(flag, true);
}

ACTION_P(SetMsg, msg)
{
	((zmq::message_t &) arg0).copy(msg);
}

/*
TEST(broker, worker_init)
{
	mock_broker_config config;
	auto sockets = std::make_shared<mock_connection_proxy>();

	zmq::message_t worker_init[] = {
		{"init", 4},
		{"env=c", 5},
		{"", 0},
		{"ignored", 7}
	};

	{
		InSequence s;

		EXPECT_CALL(*sockets, poll(_, _))
			.WillOnce(DoAll(
				ClearFlags(), SetFlag(message_receiver::WORKER)
			));

		EXPECT_CALL(*sockets, recv_workers_id(_, _))
			.WillOnce(SetArgReferee<1>("identity1"));

		EXPECT_CALL(*sockets, recv_workers(_))
			.WillOnce(Throw(zmq::error_t()));
	}

	broker<mock_connection_proxy> broker(config, sockets);
	broker.start_brokering();
}
*/
