#ifndef __M68K_INTERRUPT_RISER_H__
#define __M68K_INTERRUPT_RISER_H__

#include "m68k/cpu_registers.hpp"
#include "m68k/cpu_bus.hpp"
#include "exception_manager.h"


namespace genesis::m68k::impl
{

class interrupt_riser
{
public:
	interrupt_riser(m68k::cpu_registers& regs, m68k::cpu_bus& bus, m68k::exception_manager& exman)
		: m_regs(regs), m_bus(bus), m_exman(exman)
	{
		m_prev_ipl = m_bus.interrupt_priority();
		m_raised_ipl = 0;
	}

	void cycle()
	{
		auto ipl = m_bus.interrupt_priority();

		if(m_exman.is_raised(exception_type::interrupt))
		{
			// TODO: what if IPM changes after raising but before processing interrupt?
			if(m_raised_ipl != ipl)
			{
				// clear pending interrupt as IPL changed
				// and we haven't started processing it yet
				m_exman.accept_interrupt();
			}
		}

		if(!m_exman.is_raised(exception_type::interrupt))
		{
			if((ipl == 0b111 && m_prev_ipl != ipl) || (ipl > m_regs.flags.IPM))
			{
				m_exman.rise_interrupt(ipl);
				m_raised_ipl = ipl;
			}
		}

		m_prev_ipl = ipl;
	}

private:
	m68k::cpu_registers& m_regs;
	m68k::cpu_bus& m_bus;
	m68k::exception_manager& m_exman;
	std::uint8_t m_prev_ipl;
	std::uint8_t m_raised_ipl;
};

}

#endif // __M68K_INTERRUPT_RISER_H__
