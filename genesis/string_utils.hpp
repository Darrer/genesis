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
	ss << std::uppercase << std::hex << std::setfill('0') << std::setw(wide) << (long long)t;
	return ss.str();
}

template <class T>
std::string bin_str(T t)
{
	std::bitset<sizeof(T) * 8> bits(t);
	return bits.to_string();
}

inline void trim(std::string& str)
{
	auto not_space = [](auto c) { return !std::isspace(c); };

	// ltrim
	str.erase(str.begin(), std::find_if(str.begin(), str.end(), not_space));
	// rtrim
	str.erase(std::find_if(str.rbegin(), str.rend(), not_space).base(), str.end());
}

inline void trim(std::string_view& str)
{
	str.remove_prefix(std::min(str.find_first_not_of(' '), str.size()));

	// TODO: fixme
	auto last_space_pos = str.find_last_not_of(' ');
	if(last_space_pos != std::string_view::npos)
		str.remove_suffix(str.size() - last_space_pos - 1);
}

inline void remove_ch(std::string& str, char ch_to_remove)
{
	auto pos = str.find_first_of(ch_to_remove);

	while(pos != std::string::npos)
	{
		str.erase(pos, 1);
		pos = str.find_first_of(ch_to_remove, pos);
	}
}

} // namespace su

#endif // __STRING_UTILS_HPP__
