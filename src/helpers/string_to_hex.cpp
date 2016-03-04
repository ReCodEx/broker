#include "string_to_hex.h"

std::string helpers::string_to_hex(const std::string &string)
{
	std::stringstream ss;
	ss << std::hex << std::setfill('0') << std::nouppercase;
	for (auto &c : string) {
		ss << std::setw(2) << static_cast<unsigned int>(c);
	}
	return ss.str();
}
