#ifndef __VDP_IMPL_HV_COUNTERS_H__
#define __VDP_IMPL_HV_COUNTERS_H__

#include "vdp/settings.h"

namespace genesis::vdp::impl
{

class base_counter
{
public:
	void reset()
	{
		m_value = 0;
		m_overflow = false;
	}

	std::uint8_t value()
	{
		return m_value;
	}

protected:
	void inc2(int overflow_value, int fallback_value)
	{
		if(m_overflow && m_value == 0xFF)
			m_overflow = false;

		++m_value;
		if(!m_overflow && m_value > overflow_value)
		{
			m_value = fallback_value;
			m_overflow = true;
		}
	}

	void inc3(int overflow_value, int fallback_value)
	{
		if(m_value == 0xFF)
		{
			m_overflow = !m_overflow;
		}

		if(m_overflow && m_value == overflow_value)
		{
			m_value = fallback_value;
		}
		else
		{
			++m_value;
		}
	}

private:
	std::uint8_t m_value = 0;
	bool m_overflow = false;
};



class h_counter : public base_counter
{
public:
	void inc(display_width width)
	{
		int overflow_value = (width == display_width::c32) ? 0x93 : 0xB6;
		int fallback_value = (width == display_width::c32) ? 0xE9 : 0xE4;

		inc2(overflow_value, fallback_value);
	}
};

class v_counter : public base_counter
{
public:
	void inc(display_height height, bool PAL)
	{
		if(PAL)
		{
			if(height == display_height::c28)
				inc3(0x02, 0xCA);
			else
				inc3(0x0A, 0xD2);
		}
		else
		{
			int overflow_value = (height == display_height::c28) ? 0xEA : 0xFF;
			int fallback_value = (height == display_height::c28) ? 0xE5 : 0x00;
			inc2(overflow_value, fallback_value);
		}
	}
};

}

#endif // __VDP_IMPL_HV_COUNTERS_H__
