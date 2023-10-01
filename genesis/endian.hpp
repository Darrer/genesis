#ifndef __ENDIANNESS_HPP__
#define __ENDIANNESS_HPP__

#include <bit>
#include <cstdint>


namespace endian
{

static_assert(std::endian::native == std::endian::big || std::endian::native == std::endian::little,
			  "Mixed endianness is not supported!");


template <class T>
inline void swap(T&)
{
	// Default implementation for 1-byte types
	static_assert(sizeof(T) == 1, "not supported");
}


template <>
inline void swap<std::uint32_t>(std::uint32_t& val)
{
	val = std::byteswap(val);
}


template <>
inline void swap<std::uint16_t>(std::uint16_t& val)
{
	val = std::byteswap(val);
}


template <class T>
void big_to_sys(T& big_endian)
{
	if constexpr(std::endian::native == std::endian::little)
		swap(big_endian);
}


template <class T>
void little_to_sys(T& little_endian)
{
	if constexpr(std::endian::native == std::endian::big)
		swap(little_endian);
}


template <class T>
void sys_to_big(T& val)
{
	if constexpr(std::endian::native == std::endian::little)
		swap(val);
}


template <class T>
void sys_to_little(T& val)
{
	if constexpr(std::endian::native == std::endian::big)
		swap(val);
}


[[maybe_unused]] static std::uint8_t msb(std::uint16_t value)
{
	if constexpr(std::endian::native == std::endian::big)
		return std::uint8_t(value & 0xFF);
	// assume little endian
	return std::uint8_t(value >> 8);
}


[[maybe_unused]] static std::uint8_t lsb(std::uint16_t value)
{
	if constexpr(std::endian::native == std::endian::big)
		return std::uint8_t(value >> 8);
	// assume little endian
	return std::uint8_t(value & 0xFF);
}

} // namespace endian


#endif // __ENDIANNESS_HPP__
