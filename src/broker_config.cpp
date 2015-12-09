#include "broker_config.hpp"

broker_config::broker_config (std::istream &in)
{
}

uint16_t broker_config::get_client_port () const
{
	return 6789;
}

uint16_t broker_config::get_worker_port () const
{
	return 9876;
}
