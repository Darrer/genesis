#ifndef __ENDIANNESS_HPP__
#define __ENDIANNESS_HPP__

#include <inttypes.h>


class endianness
{
public:
	template <class T>
	static void swap(T& val) = delete;
};


template <>
void endianness::swap<uint32_t>(uint32_t& val)
{
	val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
	val = (val << 16) | (val >> 16);
}


template <>
void endianness::swap<uint16_t>(uint16_t& val)
{
	val = (val << 8) | ((val >> 8) & 0xFF);
}


#endif // __ENDIANNESS_HPP__
