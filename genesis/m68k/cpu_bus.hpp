#ifndef __M68K_CPU_BUS_HPP__
#define __M68K_CPU_BUS_HPP__

#include <cstdint>
#include <array>
#include <type_traits>
#include <algorithm>


namespace genesis::m68k
{

enum class bus
{
	/* Asynchronous bus control */
	RW,
	AS,
	UDS,
	LDS,
	DTACK,

	/* Processor status */
	FC0,
	FC1,
	FC2,

	/* System control */
	BERR,
	RESET,
	HALT,

	/* Interrupt control */
	IPL0,
	IPL1,
	IPL2,
	VPA,

	/* Bus arbitration control */
	BR,
	BG,
	// BGACK, // unsupported
};

class cpu_bus
{
private:
	static const std::size_t num_buses = 17;
	using index_type = std::underlying_type_t<bus>;

public:
	cpu_bus()
	{
		std::fill(std::begin(bus_state), std::end(bus_state), false);
	}

	void set(m68k::bus bus)
	{
		bus_state[bus_index(bus)] = true;
	}

	void clear(m68k::bus bus)
	{
		bus_state[bus_index(bus)] = false;
	}

	bool is_set(m68k::bus bus) const
	{
		return bus_state[bus_index(bus)];
	}

	void address(std::uint32_t val)
	{
		// address bus is 24 bits only, so clear high byte
		addr_bus = val & 0xFFFFFF;
	}

	std::uint32_t address() const { return addr_bus; }

	void data(std::uint16_t val) { data_bus = val; }
	std::uint16_t data() const { return data_bus; }

	void func_codes(int fc)
	{
		bus_state[bus_index(bus::FC0)] = (fc & 0b001) != 0;
		bus_state[bus_index(bus::FC1)] = (fc & 0b010) != 0;
		bus_state[bus_index(bus::FC2)] = (fc & 0b100) != 0;
	}

	int func_codes() const
	{
		int func_codes = 0;
		if(is_set(bus::FC0)) func_codes |= 0b001;
		if(is_set(bus::FC1)) func_codes |= 0b010;
		if(is_set(bus::FC2)) func_codes |= 0b100;
		return func_codes;
	}

	void interrupt_priority(std::uint8_t ipl)
	{
		bus_state[bus_index(bus::IPL0)] = (ipl & 0b001) != 0;
		bus_state[bus_index(bus::IPL1)] = (ipl & 0b010) != 0;
		bus_state[bus_index(bus::IPL2)] = (ipl & 0b100) != 0;
	}

	std::uint8_t interrupt_priority() const
	{
		std::uint8_t ipl = 0;
		if(is_set(bus::IPL0)) ipl |= 0b001;
		if(is_set(bus::IPL1)) ipl |= 0b010;
		if(is_set(bus::IPL2)) ipl |= 0b100;
		return ipl;
	}

private:
	static index_type bus_index(m68k::bus val)
	{
		return static_cast<index_type>(val);
	}

private:
	std::uint32_t addr_bus = 0;
	std::uint16_t data_bus = 0;
	bool bus_state[num_buses];
};

}

#endif // __M68K_CPU_BUS_HPP__
