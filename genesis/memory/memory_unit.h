#ifndef __MEMORY_UNIT_H__
#define __MEMORY_UNIT_H__

#include "memory/addressable.h"

#include "exception.hpp"
#include "endian.hpp"
#include "string_utils.hpp"

#include <vector>
#include <optional>

namespace genesis::memory
{

/* Generic memory unit used to represent ROM and M68K/Z80 RAM */

class memory_unit : public genesis::memory::addressable
{
public:
	/* in bytes [0 ; capacity] */
	memory_unit(std::uint32_t capacity, std::endian byte_order = std::endian::native)
		: byte_order(byte_order)
	{
		mem.resize(capacity + 1);
	}

	/* addressable interface */

	std::uint32_t capacity() const override { return mem.size(); }

	bool is_idle() const override { return true; } // always idle

	void init_write(std::uint32_t address, std::uint8_t data) override
	{
		check_addr(address, sizeof(data));

		reset();
		mem[address] = data;
	}

	void init_write(std::uint32_t address, std::uint16_t data) override
	{
		check_addr(address, sizeof(data));

		reset();

		// TODO: check read/write operations

		if (byte_order == std::endian::little)
		{
			endian::sys_to_little(data);
		}
		else if (byte_order == std::endian::big)
		{
			endian::sys_to_big(data);
		}

		mem[address] = std::uint8_t(data);
		mem[address + 1] = std::uint8_t(data >> 8);
	}

	void init_read_byte(std::uint32_t address) override
	{
		check_addr(address, 1);

		reset();

		_latched_byte = mem[address];
	}

	void init_read_word(std::uint32_t address) override
	{
		check_addr(address, 2);

		reset();

		std::uint16_t data = mem[address];
		data |= mem[address] << 8;

		// convert to sys byte order
		if (byte_order == std::endian::little)
		{
			endian::little_to_sys(data);
		}
		else if (byte_order == std::endian::big)
		{
			endian::big_to_sys(data);
		}

		_latched_word = data;
	}

	std::uint8_t latched_byte() const override { return _latched_byte.value(); }
	std::uint16_t latched_word() const override { return _latched_word.value(); }

private:
	inline void check_addr(std::uint32_t addr, size_t size)
	{
		if ((addr + size - 1) > mem.size() || addr < 0)
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

#endif // __MEMORY_UNIT_H__
