#include <vector>
#include <iostream>
#include <string>
#include "broker_core.h"


int main(int argc, char **argv)
{
	std::vector<std::string> args(argv, argv + argc);
	try {
		broker_core core(args);
		core.run();
	} catch (...) {
		std::cerr << "Something very bad happend!" << std::endl;
		return 1;
	}

	return 0;
}
