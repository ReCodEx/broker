#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>

#include "../src/reactor/reactor.h"

using namespace testing;

class pair_socket_wrapper : public socket_wrapper_base
{
public:
	pair_socket_wrapper(const std::shared_ptr<zmq::context_t> context, const std::string &addr)
		: socket_wrapper_base(context, zmq::socket_type::pair, addr, false),
		  local_socket_(*context, zmq::socket_type::pair), context_(context)
	{
		local_socket_.setsockopt(ZMQ_LINGER, 0);
		local_socket_.bind(addr_);
		socket_.close();
	}

	void initialize()
	{
		// called by the reactor
		socket_ = zmq::socket_t(*context_, zmq::socket_type::pair);
		socket_.setsockopt(ZMQ_LINGER, 0);
		socket_.connect(addr_);
	}

	bool send_message(const message_container &message)
	{
		return socket_send(socket_, message);
	}

	bool receive_message(message_container &message)
	{
		return socket_receive(socket_, message);
	}

	bool send_message_local(const message_container &message)
	{
		return socket_send(local_socket_, message);
	}

	bool receive_message_local(message_container &message)
	{
		return socket_receive(local_socket_, message, true);
	}

private:
	zmq::socket_t local_socket_;
	std::shared_ptr<zmq::context_t> context_;

	bool socket_send(zmq::socket_t &socket, const message_container &message)
	{
		bool retval;
		zmq::message_t zmessage(message.identity.c_str(), message.identity.size());
		retval = socket.send(zmessage, ZMQ_SNDMORE);

		if (!retval) {
			return false;
		}

		for (auto it = std::begin(message.data); it != std::end(message.data); ++it) {
			zmessage.rebuild(it->c_str(), it->size());
			retval = socket.send(zmessage, std::next(it) != std::end(message.data) ? ZMQ_SNDMORE : 0);

			if (!retval) {
				return false;
			}
		}

		return true;
	}

	bool socket_receive(zmq::socket_t &socket, message_container &message, bool noblock = false)
	{
		zmq::message_t received_message;
		bool retval;

		retval = socket.recv(&received_message, noblock ? ZMQ_NOBLOCK : 0);

		if (!retval) {
			return false;
		}

		message.identity = std::string((char *) received_message.data(), received_message.size());

		while (received_message.more()) {
			retval = socket.recv(&received_message);

			if (!retval) {
				return false;
			}

			message.data.emplace_back((char *) received_message.data(), received_message.size());
		}

		return true;
	}
};

/**
 * A handler driven by a function passed on creation
 */
class pluggable_handler : public handler_interface
{
public:
	typedef std::function<void(const message_container &message, response_cb response)> fnc_t;

	std::vector<message_container> received;

	pluggable_handler(fnc_t fnc) : fnc_(fnc)
	{
	}

	static std::shared_ptr<pluggable_handler> create(fnc_t fnc)
	{
		return std::make_shared<pluggable_handler>(fnc);
	}

	void on_request(const message_container &message, response_cb response)
	{
		received.push_back(message);
		fnc_(message, response);
	}

private:
	fnc_t fnc_;
};

TEST(reactor, synchronous_handler)
{
	auto context = std::make_shared<zmq::context_t>(1);
	reactor r(context);
	auto socket = std::make_shared<pair_socket_wrapper>(context, "inproc://synchronous_handler_1");
	auto handler = pluggable_handler::create([](const message_container &msg, handler_interface::response_cb respond) {
		respond(message_container("socket", "id1", {"Hello!"}));
	});

	r.add_socket("socket", socket);
	r.add_handler({"socket"}, handler);

	std::thread thread([&r]() { r.start_loop(); });
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	socket->send_message_local(message_container("", "id1", {"Hello??"}));
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	EXPECT_THAT(handler->received, ElementsAre(message_container("socket", "id1", {"Hello??"})));

	message_container message;
	socket->receive_message_local(message);

	EXPECT_EQ("id1", message.identity);
	EXPECT_THAT(message.data, ElementsAre("Hello!"));

	r.terminate();
	thread.join();
}

TEST(reactor, asynchronous_handler)
{
	auto context = std::make_shared<zmq::context_t>(1);
	reactor r(context);
	auto socket = std::make_shared<pair_socket_wrapper>(context, "inproc://asynchronous_handler_1");
	auto handler = pluggable_handler::create([](const message_container &msg, handler_interface::response_cb respond) {
		respond(message_container("socket", "id1", {"Hello!"}));
	});

	r.add_socket("socket", socket);
	r.add_async_handler({"socket"}, handler);

	std::thread thread([&r]() { r.start_loop(); });
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	socket->send_message_local(message_container("", "id1", {"Hello??"}));
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	EXPECT_THAT(handler->received, ElementsAre(message_container("socket", "id1", {"Hello??"})));

	message_container message;
	socket->receive_message_local(message);

	EXPECT_EQ("id1", message.identity);
	EXPECT_THAT(message.data, ElementsAre("Hello!"));

	r.terminate();
	thread.join();
}

TEST(reactor, timers)
{
	auto context = std::make_shared<zmq::context_t>(1);
	reactor r(context);
	auto socket = std::make_shared<pair_socket_wrapper>(context, "inproc://timers_1");
	auto handler = pluggable_handler::create([](const message_container &msg, handler_interface::response_cb respond) { });

	r.add_async_handler({r.KEY_TIMER}, handler);

	std::thread thread([&r]() { r.start_loop(); });
	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	ASSERT_GE(1, handler->received.size());

	r.terminate();
	thread.join();
}
