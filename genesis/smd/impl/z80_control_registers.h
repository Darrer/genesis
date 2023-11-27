#ifndef __SMD_IMPL_Z80_CONTROL_REGISTERS_H__
#define __SMD_IMPL_Z80_CONTROL_REGISTERS_H__

#include <memory>
#include <cstdint>

#include "memory/addressable.h"

#include <iostream>

namespace genesis::impl
{

class z80_control_registers
{
private:
	// arbitary picked value, we only need 8th bit to be cleared
	static constexpr const std::uint16_t BUS_GRANTED = 0x200;
	static constexpr const std::uint16_t BUS_REQUESTED = 0x100;
	static constexpr const std::uint16_t BUS_RELEASED = 0x0;

	static constexpr const std::uint16_t CPU_RESET_REQUESTED = 0x0;
	static constexpr const std::uint16_t CPU_RESET_CLEARED = 0x100;

public:
	z80_control_registers()
	{
		m_z80_request = std::make_shared<memory::memory_unit>(0x1, std::endian::big);
		m_z80_reset = std::make_shared<memory::memory_unit>(0x1, std::endian::big);

		reset();
	}

	// Required to Read/Write register data
	void cycle()
	{
		std::uint16_t bus_request = m_z80_request->read<std::uint16_t>(0x0);
		std::uint16_t cpu_reset = m_z80_reset->read<std::uint16_t>(0x0);

		bool bus_granted = m_z80_bus_granted;
		bool reset_requested = m_z80_reset_requested;

		// Z80 bus is requested
		if(bus_request == BUS_REQUESTED)
		{
			// clear low bit to indicate the request is granted
			m_z80_request->write<std::uint16_t>(0, BUS_GRANTED);
			m_z80_bus_granted = true;
		}
		else if(bus_request == BUS_GRANTED)
		{
			// bus is granted, wait
		}
		else if(bus_request == BUS_RELEASED)
		{
			m_z80_bus_granted = false;
		}

		// TODO: not sure how it should behave if z80 first reseted and only then bus requested

		// reset CPU only when the bus is granted
		m_z80_reset_requested = cpu_reset == CPU_RESET_REQUESTED && m_z80_bus_granted;

		/* logs */
		// if(m_z80_bus_granted == true && bus_granted == false)
		// {
		// 	std::cout << "Z80 bus requested\n";
		// }
		// else if(m_z80_bus_granted == false && bus_granted == true)
		// {
		// 	std::cout << "Z80 bus released\n";
		// }

		// if(m_z80_reset_requested == true && reset_requested == false)
		// {
		// 	std::cout << "Z80 reset requested\n";
		// }
	}

	void reset()
	{
		// At the time of power on reset access to the bus is granted
		m_z80_request->write<std::uint16_t>(0x0, BUS_GRANTED);
		m_z80_reset->write<std::uint16_t>(0x0, CPU_RESET_REQUESTED);

		m_z80_bus_granted = true;
		m_z80_reset_requested = true;
	}

	// Right now, z80 cpu does not have a bus that we can request, so use this method to determine
	// whether we need to run z80 or not
	bool z80_bus_granted() const
	{
		return m_z80_bus_granted;
	}

	bool z80_reset_requested() const
	{
		return m_z80_reset_requested;
	}

	std::shared_ptr<memory::addressable> z80_bus_request_register()
	{
		return m_z80_request;
	}

	std::shared_ptr<memory::addressable> z80_reset_register()
	{
		return m_z80_reset;
	}

private:
	bool m_z80_bus_granted;
	bool m_z80_reset_requested;
	std::shared_ptr<memory::memory_unit> m_z80_request;
	std::shared_ptr<memory::memory_unit> m_z80_reset;
};

}

#endif // __SMD_IMPL_Z80_CONTROL_REGISTERS_H__
