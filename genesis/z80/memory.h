#ifndef __Z80_MEMORY_H__
#define __Z80_MEMORY_H__

#include "exception.hpp"
#include "memory/addressable.h"
#include "memory/memory_unit.h"

#include <memory>

namespace genesis::z80
{
// using memory = genesis::memory<std::uint16_t, 0xffff, std::endian::little>;

class memory
{
public:
	using address = std::uint16_t;
	static const std::size_t max_address = 0xFFFF;

public:
	memory(std::shared_ptr<genesis::memory::addressable> addressable) : addressable(addressable)
	{
		if(addressable == nullptr)
			throw std::invalid_argument("addressable");

		// if(addressable->max_address() < 0xFFFF)
		// throw genesis::internal_error();
	}

	// Use this default constructable objects for backword compatability
	memory() : memory(std::make_shared<genesis::memory::memory_unit>(0xFFFF, std::endian::little))
	{
	}

	template <class T>
	T read(address addr)
	{
		static_assert(sizeof(T) == 1 || sizeof(T) == 2);

		// assume the result is available immediately, should be good enough for now
		if constexpr(sizeof(T) == 1)
		{
			addressable->init_read_byte(addr);
			return addressable->latched_byte();
		}
		else
		{
			addressable->init_read_word(addr);
			return addressable->latched_word();
		}
	}

	template <class T>
	void write(address addr, T data)
	{
		static_assert(sizeof(T) == 1 || sizeof(T) == 2);

		if constexpr(sizeof(T) == 1)
			addressable->init_write(addr, std::uint8_t(data));
		else
			addressable->init_write(addr, std::uint16_t(data));
	}

private:
	std::shared_ptr<genesis::memory::addressable> addressable;
};

} // namespace genesis::z80

#endif // __Z80_MEMORY_H__
