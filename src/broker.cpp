#include "broker.hpp"

broker::broker (const broker_config &config):
	config(config), context(1), clients(context, ZMQ_ROUTER), workers(context, ZMQ_DEALER)
{
}

void broker::start_brokering ()
{
	char addr[64];

	snprintf(addr, sizeof(addr), "tcp://*:%d", config.get_client_port());
	clients.bind(addr);

	snprintf(addr, sizeof(addr), "tcp://*:%d", config.get_worker_port());
	workers.bind(addr);

	zmq_pollitem_t items[] = {
			{(void *) clients, 0, ZMQ_POLLIN, 0},
			{(void *) workers, 0, ZMQ_POLLIN, 0}
	};

	while (true) {
		zmq::message_t message;

		zmq::poll(items, 2, -1);

		/* Received a message from the frontend */
		if (items[0].revents & ZMQ_POLLIN) {
			message.rebuild();
			workers.recv(&message, 0);
			std::string type((char*) message.data(), message.size());

			if (type == "eval") {
				// TODO
			}
		}

		/* Received a message from the backend */
		if (items[1].revents & ZMQ_POLLIN) {
			message.rebuild();
			workers.recv(&message, 0);
			std::string type((char*) message.data(), message.size());

			if (type == "init") {
				process_worker_init(message);
			}
		}
	}
}

void broker::process_worker_init (zmq::message_t &message)
{
	worker worker;

	while (message.more()) {
		message.rebuild();
		workers.recv(&message, 0);

		std::string header((char *) message.data(), message.size()); // TODO fix
		size_t pos = header.find('=');
		size_t value_size = header.size() - (pos + 1);
		worker.headers.emplace(header.substr(0, pos), header.substr(pos + 1, value_size));
	}

	router.add_worker(worker);
}
