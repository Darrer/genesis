#ifndef __VDP_IMPL_BLANK_FLAGS_H__
#define __VDP_IMPL_BLANK_FLAGS_H__

#include "vdp/settings.h"
#include "vdp/mode.h"

namespace genesis::vdp::impl
{

class vblank_flag
{
public:
	vblank_flag() { reset(); }

	void reset()
	{
		m_flag = false;
	}

	bool value() const { return m_flag; }

	void update(int vcounter_raw, vdp::display_height height, vdp::mode mode)
	{
		if(mode == mode::PAL)
		{
			if(height == display_height::c28)
			{
				if(vcounter_raw == 0xE0)
					m_flag = true;
				else if(vcounter_raw == 0x138)
					m_flag = false;
			}
			else
			{
				if(vcounter_raw == 0xF0)
					m_flag = true;
				else if(vcounter_raw == 0x138)
					m_flag = false;
			}
		}
		else
		{
			if(height == display_height::c28)
			{
				if(vcounter_raw == 0xE0)
					m_flag = true;
				else if(vcounter_raw == 0x105)
					m_flag = false;
			}
			else
			{
				if(vcounter_raw == 0xF0)
					m_flag = true;
				else if(vcounter_raw == 0x1FF)
					m_flag = false;
			}
		}
	}

private:
	bool m_flag;
};

class hblank_flag
{
public:
	hblank_flag() { reset(); }

	void reset()
	{
		m_flag = false;
	}

	bool value() const { return m_flag; }

	void update(int hcounter_raw, display_width width)
	{
		if(width == display_width::c32)
		{
			if(hcounter_raw == 0x93)
				m_flag = true;
			else if(hcounter_raw == 0x05)
				m_flag = false;
		}
		else
		{
			if(hcounter_raw == 0xB3)
				m_flag = true;
			else if(hcounter_raw == 0x06)
				m_flag = false;
		}
	}

private:
	bool m_flag;
};

}

#endif // __VDP_IMPL_BLANK_FLAGS_H__
