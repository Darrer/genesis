#ifndef __M68K_CPU_BUS_HPP__
#define __M68K_CPU_BUS_HPP__

#include <cstdint>
#include <array>


namespace genesis::m68k
{

enum bus : std::uint8_t
{
	/* Asynchronous bus control */
	AS,
	RW,
	UDS,
	LDS,
	DTACK,

	/* Bus arbitration control */
	BR,
	BG,
	BGACK,

	/* Interrupt control */
	IPL0,
	IPL1,
	IPL2,

	/* Processor status */
	FC0,
	FC1,
	FC2,

	/* System control */
	BERR,
	RESET,
	HALT,

	count,
};

class cpu_bus
{
public:
	cpu_bus()
	{
		bus_state.fill(false);
	}

	void set(m68k::bus bus)
	{
		bus_state.at(bus) = true;
	}

	void clear(m68k::bus bus)
	{
		bus_state.at(bus) = false;
	}

	bool is_set(m68k::bus bus) const
	{
		return bus_state.at(bus);
	}

	void address(std::uint32_t val)
	{
		// address bus is 24 bits only, so clear hight byte
		addr_bus = val & 0xFFFFFF;
	}

	std::uint32_t address() const { return addr_bus; }

	void data(std::uint16_t val) { data_bus = val; }
	std::uint16_t data() const { return data_bus; }

private:
	std::uint32_t addr_bus = 0;
	std::uint16_t data_bus = 0;
	std::array<bool, bus::count> bus_state;
};

}

#endif // __M68K_CPU_BUS_HPP__