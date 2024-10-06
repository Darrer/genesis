#ifndef __VDP_IMPL_INTERRUPT_UNIT_H__
#define __VDP_IMPL_INTERRUPT_UNIT_H__

#include "exception.hpp"
#include "vdp/m68k_interrupt_access.h"
#include "vdp/register_set.h"
#include "vdp/settings.h"

#include <iostream>
#include <limits>
#include <memory>

namespace genesis::vdp::impl
{

class interrupt_unit
{
public:
	interrupt_unit(register_set& regs, settings& sett) : m_regs(regs), m_sett(sett)
	{
		reset();
	}

	void set_m68k_interrupt_access(std::shared_ptr<m68k_interrupt_access> m68k_int)
	{
		m_m68k_int = m68k_int;
		m_m68k_int->set_interrupt_callback([this](std::uint8_t ipl) { on_interrupt(ipl); });
	}

	void reset()
	{
		m_hint_counter = 0;
		m_hint_pending = false;
		m_vint_pending = false;
		m_prev_h_counter = std::numeric_limits<int>::max();
		m_hint_raised = false;
		m_vint_raised = false;
	}

	void cycle(int raw_v_counter, int raw_h_counter)
	{
		if(m_prev_h_counter != raw_h_counter)
		{
			m_prev_h_counter = raw_h_counter;

			auto width = m_sett.display_width();
			auto height = m_sett.display_height();

			check_vint_flag(raw_v_counter, raw_h_counter, height);
			check_hint_flag(raw_v_counter, raw_v_counter, height, width);
		}

		check_interrupts();
	}

	void on_interrupt(std::uint8_t ipl)
	{
		if(ipl == 6)
		{
			// std::cout << "ACK VINT\n";
			m_vint_pending = false;
			m_regs.SR.VI = 0;
		}

		if(ipl == 4)
		{
			// std::cout << "ACK HINT\n";
			m_hint_pending = false;
		}

		m_m68k_int->interrupt_priority(0);
		m_hint_raised = false;
		m_vint_raised = false;

		check_interrupts();
	}

private:
	void check_vint_flag(int v_counter, int h_counter, display_height height)
	{
		if(m_vint_pending)
			return;

		// vint flag is set exactly at H counter 0x02
		if(h_counter != 0x02)
			return;

		if(height == display_height::c28)
		{
			if(v_counter == 0xE0)
			{
				m_vint_pending = true;
				m_regs.SR.VI = 1;
			}
		}
		else
		{
			if(v_counter == 0xF0)
			{
				m_vint_pending = true;
				m_regs.SR.VI = 1;
			}
		}
	}

	void check_hint_flag(int v_counter, int h_counter, display_height height, display_width width)
	{
		if(m_hint_counter > 0)
		{
			int max_line = height == display_height::c28 ? 0xE0 : 0xF0;
			int required_h = width == display_width::c40 ? 0xA6 : 0x86;

			if(v_counter <= max_line && h_counter == required_h)
			{
				--m_hint_counter;
				if(m_hint_counter == 0)
				{
					m_hint_pending = true;
					m_hint_counter = m_sett.horizontal_interrupt_counter();
				}
			}
		}

		if(m_regs.SR.VB == 1)
		{
			m_hint_counter = m_sett.horizontal_interrupt_counter();
		}
	}

	void check_interrupts()
	{
		if(!m_hint_raised && m_hint_pending && m_sett.horizontal_interrupt_enabled())
		{
			if(m_m68k_int == nullptr)
				throw internal_error();

			// std::cout << "Raising HINT\n";
			m_m68k_int->interrupt_priority(4);
			m_hint_raised = true;
		}

		if(!m_vint_raised && m_vint_pending && m_sett.vertical_interrupt_enabled())
		{
			if(m_m68k_int == nullptr)
				throw internal_error();

			// std::cout << "Raising VINT\n";
			m_m68k_int->interrupt_priority(6);
			m_vint_raised = true;
		}
	}

private:
	register_set& m_regs;
	settings& m_sett;

	std::shared_ptr<m68k_interrupt_access> m_m68k_int;

	int m_hint_counter;

	bool m_hint_pending;
	bool m_vint_pending;

	int m_prev_h_counter;

	bool m_hint_raised;
	bool m_vint_raised;
};

} // namespace genesis::vdp::impl

#endif // __VDP_IMPL_INTERRUPT_UNIT_H__
