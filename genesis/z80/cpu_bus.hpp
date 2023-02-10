#ifndef __CPU_BUS_HPP__
#define __CPU_BUS_HPP__

#include <cstdint>
#include <array>

namespace genesis::z80
{

// NOTE: here we assume 0 - reset, 1 - set
// for some buses that does not true, but use it for now for simplicity

enum bus : std::uint8_t
{
	INT, // maskable interrupt
	NMI, // nonmaskable interrupt
	HALT,
	BUSREQ,
	RESET,

	count,
};

class cpu_bus
{
public:
	cpu_bus()
	{
		bus_state.fill(0x0);
		data = 0x0;
	}

	void set(z80::bus bus)
	{
		bus_state.at(bus) = true;
	}

	bool is_set(z80::bus bus) const
	{
		return bus_state.at(bus);
	}

	void clear(z80::bus bus)
	{
		bus_state.at(bus) = false;
	}

	/* some specific bus */
	void set_data(std::uint8_t data)
	{
		this->data = data;
	}

	std::uint8_t get_data() const
	{
		return data;
	}

private:
	std::array<bool, z80::bus::count> bus_state;
	std::uint8_t data;
};

}

#endif // __CPU_BUS_HPP__
