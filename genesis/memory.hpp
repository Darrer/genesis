#ifndef __MEMORY_HPP__
#define __MEMORY_HPP__

#include "endian.hpp"

#include <array>
#include <cstddef>
#include <limits>
#include <stdexcept>


namespace genesis
{

template <class address, size_t capacity /* in bytes */, std::endian byte_order>
class memory
{
public:
	memory()
	{
		static_assert(std::numeric_limits<address>::max() >= capacity,
					  "address type must be larget enough to address capacity!");

		static_assert(byte_order == std::endian::little || byte_order == std::endian::big);
	}

	template <class T>
	T read(address addr)
	{
		check_addr(addr, sizeof(T));

		T data{};

		for (size_t i = 0; i < sizeof(T); ++i)
		{
			std::byte* b = reinterpret_cast<std::byte*>(&data) + i;
			*b = mem[addr + i];
		}

		// convert to sys byte order
		if constexpr (byte_order == std::endian::little)
		{
			endian::little_to_sys(data);
		}
		else
		{
			endian::big_to_sys(data);
		}

		return data;
	}

	template <class T>
	void write(address addr, T data)
	{
		check_addr(addr, sizeof(T));

		// convert to proper byte order
		if constexpr (byte_order == std::endian::little)
		{
			endian::sys_to_little(data);
		}
		else
		{
			endian::sys_to_big(data);
		}

		for (size_t i = 0; i < sizeof(T); ++i)
			mem[addr + i] = *(reinterpret_cast<std::byte*>(&data) + i);
	}

private:
	inline void check_addr(address addr, size_t size)
	{
		if (addr + size >= capacity || addr < 0)
			throw std::runtime_error("memory check: wrong address");
	}

private:
	std::array<std::byte, capacity> mem;
};

} // namespace genesis
#endif //__MEMORY_HPP__
