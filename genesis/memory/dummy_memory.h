#ifndef __MEMORY_DUMMY_MEMORY_H__
#define __MEMORY_DUMMY_MEMORY_H__

#include "addressable.h"

namespace genesis::memory
{

class dummy_memory : public memory::addressable
{
public:
	/* in bytes [0 ; highest_address] */
	dummy_memory(std::uint32_t highest_address, std::endian byte_order = std::endian::native)
		: highest_address(highest_address)
	{
	}

	/* addressable interface */

	std::uint32_t max_address() const override { return highest_address; }

	bool is_idle() const override { return true; } // always idle

	void init_write(std::uint32_t /* address */, std::uint8_t /* data */) override
	{
		reset();
	}

	void init_write(std::uint32_t /* address */, std::uint16_t /* data */) override
	{
		reset();
	}

	void init_read_byte(std::uint32_t address) override
	{
		reset();
		_latched_byte = next_8++;
	}

	void init_read_word(std::uint32_t address) override
	{
		reset();
		_latched_word = next_16++;
	}

	std::uint8_t latched_byte() const override { return _latched_byte.value(); }
	std::uint16_t latched_word() const override { return _latched_word.value(); }

private:
	void reset()
	{
		_latched_byte.reset();
		_latched_word.reset();
	}

private:
	std::uint32_t highest_address;
	std::uint8_t next_8 = 0;
	std::uint16_t next_16 = 0;
	std::optional<std::uint8_t> _latched_byte;
	std::optional<std::uint16_t> _latched_word;
};

template<std::uint8_t read8_value, std::uint16_t read16_value>
class constant_memory_unit : public memory::addressable
{
public:
	/* in bytes [0 ; highest_address] */
	constant_memory_unit(std::uint32_t highest_address, std::endian byte_order = std::endian::native)
		: highest_address(highest_address)
	{
	}

	/* addressable interface */

	std::uint32_t max_address() const override { return highest_address; }

	bool is_idle() const override { return true; } // always idle

	void init_write(std::uint32_t /* address */, std::uint8_t /* data */) override
	{
	}

	void init_write(std::uint32_t /* address */, std::uint16_t /* data */) override
	{
	}

	void init_read_byte(std::uint32_t /* address */) override
	{
	}

	void init_read_word(std::uint32_t /* address */) override
	{
	}

	std::uint8_t latched_byte() const override { return read8_value; }
	std::uint16_t latched_word() const override { return read16_value; }

private:
	std::uint32_t highest_address;
};

using zero_memory_unit = constant_memory_unit<0x00, 0x0000>;
using ffff_memory_unit = constant_memory_unit<0xFF, 0xFFFF>;

}

#endif // __MEMORY_DUMMY_MEMORY_H__
