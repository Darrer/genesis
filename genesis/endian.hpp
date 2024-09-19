#ifndef __ENDIANNESS_HPP__
#define __ENDIANNESS_HPP__

#include <bit>
#include <cstdint>


namespace endian
{

static_assert(std::endian::native == std::endian::big || std::endian::native == std::endian::little,
			  "Mixed endianness is not supported!");


template <class T>
inline void swap(T& val)
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


[[maybe_unused]] static void swap_nibbles(std::uint8_t& val)
{
	val = (val >> 4) | (val << 4);
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


[[maybe_unused]] static std::uint16_t msw(std::uint32_t value)
{
	if constexpr(std::endian::native == std::endian::big)
		return std::uint16_t(value & 0xFFFF);
	// assume little endian
	return std::uint16_t(value >> 16);
}


[[maybe_unused]] static std::uint16_t lsw(std::uint32_t value)
{
	if constexpr(std::endian::native == std::endian::big)
		return std::uint16_t(value >> 16);
	// assume little endian
	return std::uint16_t(value & 0xFFFF);
}

} // namespace endian


#endif // __ENDIANNESS_HPP__
