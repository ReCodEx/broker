#include <thread>

#include "reactor.h"

const std::string reactor::KEY_TIMER = "timer";

reactor::reactor(std::shared_ptr<zmq::context_t> context)
	: context_(context), async_handler_socket_(*context, zmq::socket_type::router),
	  unique_id("reactor_" + std::to_string((uintptr_t) this))
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

	// Enter the poll loop
	while (true) {
		auto time_before_poll = std::chrono::system_clock::now();
		zmq::poll(pollitems, std::chrono::milliseconds(100));
		auto time_after_poll = std::chrono::system_clock::now();

		auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(time_after_poll - time_before_poll);

		size_t i = 0;
		for (auto item : pollitems) {
			if (item.revents & ZMQ_POLLIN) {
				message_container received_msg;
				received_msg.key = pollitem_names.at(i);
				sockets_.at(received_msg.key)->receive_message(received_msg);
				process_message(received_msg);
			}

			++i;
		}

		message_container timer_msg;
		timer_msg.key = KEY_TIMER;
		timer_msg.data.push_back(std::to_string(elapsed_time.count()));

		process_message(timer_msg);
	}
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
	: handler_wrapper(reactor_ref, handler), reactor_socket_(async_handler_socket),
	  unique_id_(std::to_string((uintptr_t) this)), handler_thread_socket_(context, zmq::socket_type::dealer),
	  worker_([this]() { handler_thread(); })
{
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
	handler_thread_socket_.setsockopt(ZMQ_IDENTITY, unique_id_.data(), unique_id_.size());
	handler_thread_socket_.connect("inproc://" + reactor_.unique_id);

	while (true) {
		message_container request;
		zmq::message_t message;

		handler_thread_socket_.recv(&message, 0);
		request.key = std::string(static_cast<char *>(message.data()), message.size());

		if (request.key == TERMINATE_MSG) {
			return;
		}

		if (!message.more()) {
			continue;
		}

		handler_thread_socket_.recv(&message, 0);
		request.identity = std::string(static_cast<char *>(message.data()), message.size());

		while (message.more()) {
			handler_thread_socket_.recv(&message, 0);
			request.data.emplace_back(static_cast<char *>(message.data()), message.size());
		}

		handler_->on_request(request, [this](const message_container &response) { send_response(response); });
	}
}

void asynchronous_handler_wrapper::send_response(const message_container &message)
{
	handler_thread_socket_.send(unique_id_.data(), unique_id_.size(), ZMQ_SNDMORE);
	handler_thread_socket_.send(message.key.data(), message.key.size(), ZMQ_SNDMORE);
	handler_thread_socket_.send(message.identity.data(), message.identity.size(), ZMQ_SNDMORE);

	for (auto it = std::begin(message.data); it != std::end(message.data); ++it) {
		handler_thread_socket_.send(it->c_str(), it->size(), std::next(it) != std::end(message.data) ? ZMQ_SNDMORE : 0);
	}
}
