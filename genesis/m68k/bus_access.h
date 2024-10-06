#ifndef __M68K_BUS_ACCESS_H__
#define __M68K_BUS_ACCESS_H__

#include "impl/bus_manager.h"

#include <cstdint>
#include <source_location>

namespace genesis::m68k
{

/**
 * Provides API to m68k bus for external CPU clients.
 * All requests are redirected to m68k::bus_manager.
 */

class bus_access
{
public:
	bus_access(m68k::bus_manager& busm) : busm(busm)
	{
	}

	bool is_idle() const
	{
		return busm.is_idle();
	}

	/* read/write interface */

	template <class Callable = std::nullptr_t>
	void init_write(std::uint32_t address, std::uint8_t data, Callable cb = nullptr)
	{
		assert_bus_granted();
		busm.init_write(address, data, cb);
	}

	template <class Callable = std::nullptr_t>
	void init_write(std::uint32_t address, std::uint16_t data, Callable cb = nullptr)
	{
		assert_bus_granted();
		busm.init_write(address, data, cb);
	}

	template <class Callable = std::nullptr_t>
	void init_read_byte(std::uint32_t address, addr_space space, Callable cb = nullptr)
	{
		assert_bus_granted();
		busm.init_read_byte(address, space, cb);
	}

	template <class Callable = std::nullptr_t>
	void init_read_word(std::uint32_t address, addr_space space, Callable cb = nullptr)
	{
		assert_bus_granted();
		busm.init_read_word(address, space, cb);
	}

	std::uint8_t latched_byte() const
	{
		return busm.latched_byte();
	}

	std::uint16_t latched_word() const
	{
		return busm.latched_word();
	}

	/* bus arbitration interface */

	bool bus_granted() const
	{
		return busm.bus_granted();
	}

	void request_bus()
	{
		busm.request_bus();
	}

	void release_bus()
	{
		busm.release_bus();
	}

private:
	void assert_bus_granted(std::source_location loc = std::source_location::current())
	{
		if(!bus_granted())
			throw internal_error(std::string("Illegal bus operation in ") + loc.function_name());
	}

private:
	m68k::bus_manager& busm;
};

} // namespace genesis::m68k

#endif // __M68K_BUS_ACCESS_H__
