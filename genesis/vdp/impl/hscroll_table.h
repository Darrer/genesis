#ifndef __VDP_IMPL_HSCROLL_TABLE_H__
#define __VDP_IMPL_HSCROLL_TABLE_H__

#include <cstdint>
#include <stdexcept>

#include "vdp/memory.h"
#include "vdp/settings.h"
#include "plane_type.h"


namespace genesis::vdp::impl
{

// horizontal scroll table
class hscroll_table
{
public:
	hscroll_table(plane_type plane, genesis::vdp::settings& sett, genesis::vdp::vram_t& vram)
		: plane(plane), sett(sett), vram(vram)
	{
		if(plane != plane_type::a && plane != plane_type::b)
			throw std::invalid_argument("plane");
	}

	// returns horizontal scroll offset for specified row_number
	int get_offset(unsigned row_number) const
	{
		if(row_number >= sett.display_height_in_pixels())
			throw genesis::internal_error();

		std::uint16_t offset = vram.read<std::uint16_t>(entry_address(row_number));
		return offset & 0b1111111111;
	}

private:
	std::uint32_t entry_address(unsigned row_number) const
	{
		std::uint32_t address = sett.horizontal_scroll_address();
		switch (sett.horizontal_scrolling())
		{
		case horizontal_scrolling::full_screen:
			break;

		case horizontal_scrolling::line:
			// * 2 - as there are 2 A & B plane entries
			// * 2 - as every entry is 2 bytes width
			address += row_number * 4;
			break;

		case horizontal_scrolling::cell:
			// * 16 - as there are 16 elements between entries
			// * 2 - as every entry is 2 bytes width
			// TODO: not checked
			// tail_number = row_number / 8
			// address += tail_number * 32

			address += (row_number / 8) * 32;
			// address += (row_number & ~0b111) * 32;
			break;
		
		case horizontal_scrolling::invalid:
			// TODO:
			// undocumented behavior
			break;

		default: throw internal_error();
		}

		if(plane == plane_type::b)
			address += 2;
		
		return address;
	}

private:
	plane_type plane;
	genesis::vdp::settings& sett;
	genesis::vdp::vram_t& vram;
};

}

#endif // __VDP_IMPL_HSCROLL_TABLE_H__
