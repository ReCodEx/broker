#include <iostream>

#include "task_router.hpp"
#include "broker_config.hpp"
#include "broker.hpp"

int main (int argc, char **argv)
{
	broker_config config(std::cin);
	broker broker(config);

	broker.start_brokering();

	return 0;
}