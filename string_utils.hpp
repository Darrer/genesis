#ifndef __STRING_UTILS_HPP__
#define __STRING_UTILS_HPP__

#include <sstream>
#include <string>


template <class T> std::string hex_str(T t, size_t wide = sizeof(T) * 2)
{
	std::stringstream ss;
	ss << "0x";
	ss << std::hex << std::setfill('0') << std::setw(wide);
	ss << t;
	return ss.str();
}

#endif // __STRING_UTILS_HPP__
