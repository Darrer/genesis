#ifndef __MEMORY_HPP__
#define __MEMORY_HPP__

#include "endian.hpp"
#include "string_utils.hpp"

#include <array>
#include <cstddef>
#include <limits>
#include <stdexcept>


namespace genesis
{

template <class address_t, size_t capacity /* in bytes */, std::endian byte_order>
class memory
{
public:
	using address = address_t;
	static const std::size_t max_capacity = capacity;

	memory()
	{
		static_assert(std::numeric_limits<address>::max() >= capacity,
					  "address type must be big enough to address capacity!");

		static_assert(byte_order == std::endian::little || byte_order == std::endian::big);

		for (auto& b : mem)
			b = (std::byte)0xFF;
	}

	template <class T>
	T read(address addr)
	{
		check_addr(addr, sizeof(T));

		T data = *reinterpret_cast<T*>(&mem[addr]);

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
		if (addr + size > capacity || addr < 0)
			throw std::runtime_error("memory check: wrong address (" + su::hex_str(addr) +
									 ") read: " + std::to_string(size));
	}

private:
	std::array<std::byte, capacity + 1> mem;
};

} // namespace genesis
#endif //__MEMORY_HPP__
