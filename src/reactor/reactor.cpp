#include <thread>

#include "reactor.h"

const std::string reactor::KEY_TIMER = "timer";

reactor::reactor(std::shared_ptr<zmq::context_t> context)
	: unique_id("reactor_" + std::to_string((uintptr_t) this)), context_(context),
	  async_handler_socket_(*context, zmq::socket_type::router)
{
	async_handler_socket_.bind("inproc://" + unique_id);
}

void reactor::add_socket(const std::string &name, std::shared_ptr<socket_wrapper_base> socket)
{
	sockets_.emplace(name, socket);
}

void reactor::add_handler(const std::vector<std::string> &origins, std::shared_ptr<handler_interface> handler)
{
	auto wrapper = std::make_shared<handler_wrapper>(*this, handler);

	for (auto &origin : origins) {
		handlers_.emplace(origin, wrapper);
	}
}

void reactor::add_async_handler(const std::vector<std::string> &origins, std::shared_ptr<handler_interface> handler)
{
	auto wrapper = std::make_shared<asynchronous_handler_wrapper>(*context_, async_handler_socket_, *this, handler);

	for (auto &origin : origins) {
		handlers_.emplace(origin, wrapper);
	}
}

void reactor::send_message(const message_container &message)
{
	auto it = sockets_.find(message.key);

	// If there is a matching socket, send the message there. If not, let the reactor handle it
	if (it != std::end(sockets_)) {
		it->second->send_message(message);
	} else {
		process_message(message);
	}
}

void reactor::process_message(const message_container &message)
{
	auto range = handlers_.equal_range(message.key);

	for (auto it = range.first; it != range.second; ++it) {
		(*it->second)(message);
	}
}

void reactor::start_loop()
{
	std::vector<zmq::pollitem_t> pollitems;
	std::vector<std::string> pollitem_names;

	// Poll all registered sockets
	for (auto it : sockets_) {
		it.second->initialize();
		pollitems.push_back(it.second->get_pollitem());
		pollitem_names.push_back(it.first);
	}

	// Also poll the internal socket for asynchronous communication
	pollitems.push_back(
		zmq_pollitem_t{.socket = (void *) async_handler_socket_, .fd = 0, .events = ZMQ_POLLIN, .revents = 0});

	termination_flag_.store(false);

	// Enter the poll loop
	while (!termination_flag_.load()) {
		auto time_before_poll = std::chrono::system_clock::now();
		zmq::poll(pollitems, std::chrono::milliseconds(100));
		auto time_after_poll = std::chrono::system_clock::now();

		auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(time_after_poll - time_before_poll);

		size_t i = 0;
		for (auto item : pollitems) {
			if (item.revents & ZMQ_POLLIN) {
				message_container received_msg;

				if (i < pollitem_names.size()) {
					// message came from a registered socket, fill in its key
					received_msg.key = pollitem_names.at(i);
					sockets_.at(received_msg.key)->receive_message(received_msg);

					// messages from sockets must go through a handler
					process_message(received_msg);
				} else {
					// message from the asynchronous handler socket
					zmq::message_t zmessage;

					// this is the async handler identity - we don't care about that
					async_handler_socket_.recv(&zmessage);

					async_handler_socket_.recv(&zmessage);
					received_msg.key = std::string(static_cast<char *>(zmessage.data()), zmessage.size());

					async_handler_socket_.recv(&zmessage);
					received_msg.identity = std::string(static_cast<char *>(zmessage.data()), zmessage.size());

					while (zmessage.more()) {
						async_handler_socket_.recv(&zmessage);
						received_msg.data.emplace_back(static_cast<char *>(zmessage.data()), zmessage.size());
					}

					// messages from async handlers might be destined to a socket
					send_message(received_msg);
				}
			}

			++i;
		}

		message_container timer_msg;
		timer_msg.key = KEY_TIMER;
		timer_msg.data.push_back(std::to_string(elapsed_time.count()));

		process_message(timer_msg);
	}

	handlers_.clear();
}

void reactor::terminate()
{
	termination_flag_.store(true);
}

handler_wrapper::handler_wrapper(reactor &reactor_ref, std::shared_ptr<handler_interface> handler)
	: handler_(handler), reactor_(reactor_ref)
{
}

handler_wrapper::~handler_wrapper()
{
}

void handler_wrapper::operator()(const message_container &message)
{
	handler_->on_request(message, [this](const message_container &response) { reactor_.send_message(response); });
}

const std::string asynchronous_handler_wrapper::TERMINATE_MSG = "TERMINATE";

asynchronous_handler_wrapper::asynchronous_handler_wrapper(zmq::context_t &context,
	zmq::socket_t &async_handler_socket,
	reactor &reactor_ref,
	std::shared_ptr<handler_interface> handler)
	: handler_wrapper(reactor_ref, handler), context_(context),
	  reactor_socket_(async_handler_socket), unique_id_(std::to_string((uintptr_t) this))
{
	worker_ = std::thread([this]() { handler_thread(); });
}

asynchronous_handler_wrapper::~asynchronous_handler_wrapper()
{
	// Send a message to terminate the worker thread
	reactor_socket_.send(unique_id_.data(), unique_id_.size(), ZMQ_SNDMORE);
	reactor_socket_.send(TERMINATE_MSG.data(), TERMINATE_MSG.size());

	// Join it
	worker_.join();
}

void asynchronous_handler_wrapper::operator()(const message_container &message)
{
	reactor_socket_.send(unique_id_.data(), unique_id_.size(), ZMQ_SNDMORE);
	reactor_socket_.send(message.key.data(), message.key.size(), ZMQ_SNDMORE);
	reactor_socket_.send(message.identity.data(), message.identity.size(), ZMQ_SNDMORE);

	for (auto it = std::begin(message.data); it != std::end(message.data); ++it) {
		reactor_socket_.send(it->c_str(), it->size(), std::next(it) != std::end(message.data) ? ZMQ_SNDMORE : 0);
	}
}

void asynchronous_handler_wrapper::handler_thread()
{
	zmq::socket_t socket(context_, zmq::socket_type::dealer);
	socket.setsockopt(ZMQ_IDENTITY, unique_id_.data(), unique_id_.size());
	socket.connect("inproc://" + reactor_.unique_id);

	while (true) {
		message_container request;
		zmq::message_t message;

		socket.recv(&message, 0);
		request.key = std::string(static_cast<char *>(message.data()), message.size());

		if (request.key == TERMINATE_MSG) {
			return;
		}

		if (!message.more()) {
			continue;
		}

		socket.recv(&message, 0);
		request.identity = std::string(static_cast<char *>(message.data()), message.size());

		while (message.more()) {
			socket.recv(&message, 0);
			request.data.emplace_back(static_cast<char *>(message.data()), message.size());
		}

		handler_->on_request(
			request, [this, &socket](const message_container &response) { send_response(socket, response); });
	}
}

void asynchronous_handler_wrapper::send_response(zmq::socket_t &socket, const message_container &message)
{
	socket.send(message.key.data(), message.key.size(), ZMQ_SNDMORE);
	socket.send(message.identity.data(), message.identity.size(), ZMQ_SNDMORE);

	for (auto it = std::begin(message.data); it != std::end(message.data); ++it) {
		socket.send(it->c_str(), it->size(), std::next(it) != std::end(message.data) ? ZMQ_SNDMORE : 0);
	}
}
