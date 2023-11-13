#ifndef __DUMMY_MEMORY_H__
#define __DUMMY_MEMORY_H__

#include "addressable.h"

namespace genesis::memory
{

class dummy_memory : public memory::addressable
{
public:
	/* in bytes [0 ; highest_address] */
	dummy_memory(std::uint32_t highest_address, std::endian byte_order = std::endian::native)
		: highest_address(highest_address), byte_order(byte_order)
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

	void init_read_byte(std::uint32_t /* address */) override
	{
		reset();
		_latched_byte = (next_data++) & 0xFF;
	}

	void init_read_word(std::uint32_t /* address */) override
	{
		reset();
		_latched_word = next_data++;
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
	std::endian byte_order;
	std::uint16_t next_data = 0;
	std::optional<std::uint8_t> _latched_byte;
	std::optional<std::uint16_t> _latched_word;
};

}

#endif // __SMD_IMPL_DUMMY_MEMORY_H__
