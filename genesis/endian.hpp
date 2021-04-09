#ifndef __ENDIANNESS_HPP__
#define __ENDIANNESS_HPP__

#include <bit>

// TODO: rewrite using return value?

namespace endian
{

static_assert(std::endian::native == std::endian::big || std::endian::native == std::endian::little,
			  "Mixed endianness is not supported!");


template <class T>
inline void swap(T& val) = delete;


template <>
inline void swap<std::uint32_t>(std::uint32_t& val)
{
	val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
	val = (val << 16) | (val >> 16);
}


template <>
inline void swap<std::uint16_t>(std::uint16_t& val)
{
	val = (val << 8) | ((val >> 8) & 0xFF);
}


template <>
inline void swap<std::uint8_t>(std::uint8_t&)
{
}


template <class T>
void big_to_sys(T& big_endian)
{
	if constexpr (std::endian::native == std::endian::little)
		swap(big_endian);
}


template <class T>
void little_to_sys(T& little_endian)
{
	if constexpr (std::endian::native == std::endian::big)
		swap(little_endian);
}


template <class T>
void sys_to_big(T& val)
{
	if constexpr (std::endian::native == std::endian::little)
		swap(val);
}


template <class T>
void sys_to_little(T& val)
{
	if constexpr (std::endian::native == std::endian::big)
		swap(val);
}

} // namespace endian


#endif // __ENDIANNESS_HPP__
