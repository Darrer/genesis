#ifndef __MEMORY_MEMORY_UNIT_H__
#define __MEMORY_MEMORY_UNIT_H__

#include "memory/addressable.h"

#include "exception.hpp"
#include "endian.hpp"
#include "string_utils.hpp"

#include <vector>
#include <optional>

namespace genesis::memory
{

/* Generic memory unit used to represent ROM and M68K/Z80 RAM */

class memory_unit : public addressable
{
public:
	/* in bytes [0 ; highest_address] */
	memory_unit(std::uint32_t highest_address, std::endian byte_order = std::endian::native)
		: byte_order(byte_order)
	{
		mem.resize(highest_address + 1); // + 1 to account for 0 index
	}

	/* addressable interface */

	std::uint32_t max_address() const override { return static_cast<std::uint32_t>(mem.size()) - 1; }

	bool is_idle() const override { return true; } // always idle

	void init_write(std::uint32_t address, std::uint8_t data) override
	{
		reset();
		write(address, data);
	}

	void init_write(std::uint32_t address, std::uint16_t data) override
	{
		reset();
		write(address, data);
	}

	void init_read_byte(std::uint32_t address) override
	{
		reset();
		_latched_byte = read<std::uint8_t>(address);
	}

	void init_read_word(std::uint32_t address) override
	{
		reset();
		_latched_word = read<std::uint16_t>(address);
	}

	std::uint8_t latched_byte() const override { return _latched_byte.value(); }
	std::uint16_t latched_word() const override { return _latched_word.value(); }

	/* direct interface */

	template<class T>
	void write(std::uint32_t address, T data)
	{
		check_addr(address, sizeof(T));

		// convert to proper byte order
		if (byte_order == std::endian::little)
		{
			endian::sys_to_little(data);
		}
		else if (byte_order == std::endian::big)
		{
			endian::sys_to_big(data);
		}

		for (size_t i = 0; i < sizeof(T); ++i)
		{
			// check_addr should properly handle out-of-boundary array accesses,
			// but gcc still rises false-positive warnings
			// so use std::array::at to calm gcc down
			mem.at(address + i) = *(reinterpret_cast<std::uint8_t*>(&data) + i);
		}
	}

	template<class T>
	T read(std::uint32_t address)
	{
		check_addr(address, sizeof(T));

		T data = *reinterpret_cast<T*>(&mem[address]);

		// convert to sys byte order
		if (byte_order == std::endian::little)
		{
			endian::little_to_sys(data);
		}
		else if (byte_order == std::endian::big)
		{
			endian::big_to_sys(data);
		}

		return data;
	}

private:
	inline void check_addr(std::uint32_t addr, size_t size)
	{
		if(addr > max_address() || (addr + size - 1) > max_address())
			throw internal_error("memory_unit check: wrong address (" + su::hex_str(addr) +
									 ") size: " + std::to_string(size));
	}

	void reset()
	{
		_latched_byte.reset();
		_latched_word.reset();
	}

private:
	std::vector<std::uint8_t> mem;
	std::endian byte_order;

	std::optional<std::uint8_t> _latched_byte;
	std::optional<std::uint16_t> _latched_word;
};

}

#endif // __MEMORY_MEMORY_UNIT_H__
