#ifndef __VDP_IMPL_VSCROLL_TABLE_H__
#define __VDP_IMPL_VSCROLL_TABLE_H__

#include <cstdint>
#include <stdexcept>

#include "vdp/memory.h"
#include "vdp/settings.h"
#include "plane_type.h"

namespace genesis::vdp::impl
{

class vscroll_table
{
public:
	static int get_offset(plane_type plane_type, unsigned tail_column_number, vdp::settings& sett, vdp::vsram_t& vsram)
	{
		if(plane_type != plane_type::a && plane_type != plane_type::b)
			throw genesis::internal_error();

		std::uint16_t offset = vsram.read(get_address(plane_type, tail_column_number, sett));
		return offset & 0b1111111111;
	}

private:
	static std::uint16_t get_address(plane_type plane_type, unsigned tail_column_number, vdp::settings& sett)
	{
		std::uint16_t address = 0;
		switch (sett.vertical_scrolling())
		{
		case vertical_scrolling::full_screen:
			break;

		case vertical_scrolling::two_cell:
		{
			int strip = (tail_column_number & ~1) % num_strips(sett);
			// 2 bytes for each A/B plane
			address += strip * 4;
			break;
		}
		
		default: throw genesis::internal_error();
		}

		if(plane_type == plane_type::b)
			address += 2;

		return address;
	}

	static int num_strips(vdp::settings& sett)
	{
		// 20 strips in 320 pixels mode
		// 16 strips in 256 pixels mode
		if(sett.display_width() == display_width::c40)
			return 20;
		return 16;
	}
};

}

#endif // __VDP_IMPL_VSCROLL_TABLE_H__
