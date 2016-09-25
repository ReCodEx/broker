#include <thread>

#include "reactor.h"

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
				sockets_.at(pollitem_names.at(i))->receive_message(received_msg);
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

asynchronous_handler_wrapper::asynchronous_handler_wrapper(zmq::context_t &context,
	zmq::socket_t &async_handler_socket,
	reactor &reactor_ref,
	std::shared_ptr<handler_interface> handler)
	: handler_wrapper(reactor_ref, handler), reactor_socket_(async_handler_socket),
	  unique_id_(std::to_string((uintptr_t) this)), handler_thread_socket_(context, zmq::socket_type::dealer)
{
	std::thread worker([this]() { handler_thread(); });
}

asynchronous_handler_wrapper::~asynchronous_handler_wrapper()
{
	// TODO find a way to kill the handling thread
}

void asynchronous_handler_wrapper::operator()(const message_container &message)
{
	// TODO send message through reactor_socket_
}

void asynchronous_handler_wrapper::handler_thread()
{
	handler_thread_socket_.setsockopt(ZMQ_IDENTITY, unique_id_.data(), unique_id_.size());
	handler_thread_socket_.connect("inproc://" + reactor_.unique_id);

	while (true) {
		message_container message;

		// TODO receive message from handler_thread_socket_

		handler_->on_request(message, [this] (const message_container &response) { send_response(response); });
	}
}

void asynchronous_handler_wrapper::send_response(const message_container &message)
{
	// TODO send a message back through handler_thread_socket_
}
