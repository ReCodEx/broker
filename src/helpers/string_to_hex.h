#ifndef RECODEX_BROKER_STRING_TO_HEX_H
#define RECODEX_BROKER_STRING_TO_HEX_H

#include <iomanip>
#include <sstream>
#include <string>

namespace helpers
{
	/**
	 * Converts all characters to its hexadecimal representation and return it in form of textual concatenation.
	 * @param string text which will be converted to its binary representation
	 * @return textual description of hexadecimal characters from given string
	 */
	std::string string_to_hex(const std::string &string);
}


#endif // RECODEX_BROKER_STRING_TO_HEX_H
