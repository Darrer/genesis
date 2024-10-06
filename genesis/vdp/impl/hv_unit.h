#ifndef __VDP_IMPL_HV_UNIT_H__
#define __VDP_IMPL_HV_UNIT_H__

#include "blank_flags.h"
#include "hv_counters.h"
#include "vdp/mode.h"
#include "vdp/register_set.h"
#include "vdp/settings.h"


namespace genesis::vdp::impl
{

/* This class updates H/V counters and flags. */
class hv_unit
{
public:
	hv_unit(register_set& regs) : m_regs(regs)
	{
	}

	void reset()
	{
		m_h_counter.reset();
		m_v_counter.reset();

		m_hblank_flag.reset();
		m_vblank_flag.reset();
	}

	int h_counter_raw() const
	{
		return m_h_counter.raw_value();
	}
	int v_counter_raw() const
	{
		return m_v_counter.raw_value();
	}

	void on_pixel(display_width width, display_height height, mode mode)
	{
		/* update H counter every pixel */
		m_h_counter.inc(width);
		m_regs.h_counter = m_h_counter.value();

		m_hblank_flag.update(m_h_counter.raw_value(), width);
		m_regs.SR.HB = m_hblank_flag.value() ? 1 : 0;

		/* update V counter at specific H position */
		if((width == display_width::c32 && m_h_counter.raw_value() == 0x85) ||
		   (width == display_width::c40 && m_h_counter.raw_value() == 0xA5))
		{
			m_v_counter.inc(height, mode);
			m_regs.v_counter = m_v_counter.value();

			m_vblank_flag.update(m_v_counter.raw_value(), height, mode);
			m_regs.SR.VB = m_vblank_flag.value() ? 1 : 0;
		}
	}

private:
	register_set& m_regs;

	h_counter m_h_counter;
	v_counter m_v_counter;

	hblank_flag m_hblank_flag;
	vblank_flag m_vblank_flag;
};

} // namespace genesis::vdp::impl

#endif // __VDP_IMPL_HV_UNIT_H__
