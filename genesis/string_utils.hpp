#ifndef __STRING_UTILS_HPP__
#define __STRING_UTILS_HPP__

#include <algorithm>
#include <bitset>
#include <iomanip>
#include <sstream>
#include <string>


// string utils
namespace su
{

template <class T>
std::string hex_str(T t, size_t wide = sizeof(T) * 2)
{
	std::stringstream ss;
	ss << "0x";
	ss << std::hex << std::setfill('0') << std::setw(wide) << (long long)t;
	return ss.str();
}


template <class T>
std::string bin_str(T t)
{
	const size_t num_bits = sizeof(T) * 8;
	std::bitset<num_bits> bits(t);

	std::stringstream ss;
	for (size_t i = 1; i <= num_bits; ++i)
	{
		if (bits.test(num_bits - i))
			ss << '1';
		else
			ss << '0';
	}

	return ss.str();
}


inline void trim(std::string& str)
{
	auto not_space = [](auto c) { return !std::isspace(c); };

	// ltrim
	str.erase(str.begin(), std::find_if(str.begin(), str.end(), not_space));
	// rtrim
	str.erase(std::find_if(str.rbegin(), str.rend(), not_space).base(), str.end());
}

} // namespace su

#endif // __STRING_UTILS_HPP__
