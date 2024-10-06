#ifndef __VDP_IMPL_HSCROLL_TABLE_H__
#define __VDP_IMPL_HSCROLL_TABLE_H__

#include "plane_type.h"
#include "vdp/memory.h"
#include "vdp/settings.h"

#include <cstdint>
#include <stdexcept>


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

	// returns horizontal scroll offset for specified line_number
	int get_offset(unsigned line_number) const
	{
		if(line_number >= sett.display_height_in_pixels())
			throw genesis::internal_error();

		std::uint16_t offset = vram.read<std::uint16_t>(entry_address(line_number));
		return offset & 0b1'111'111'111;
	}

private:
	std::uint32_t entry_address(unsigned line_number) const
	{
		std::uint32_t address = sett.horizontal_scroll_address();
		switch(sett.horizontal_scrolling())
		{
		case horizontal_scrolling::full_screen:
			break;

		case horizontal_scrolling::line:
			// * 2 - as there are 2 A & B plane entries
			// * 2 - as every entry is 2 bytes width
			address += line_number * 4;
			break;

		case horizontal_scrolling::cell:
			// * 16 - as there are 16 elements between entries
			// * 2 - as every entry is 2 bytes width
			// * /8 (or >> 3) - to get a strip/cell number

			address += (line_number >> 3) * 32;
			break;

		case horizontal_scrolling::invalid:
			// undocumented behavior
			// TODO: not sure about this behavior
			address += (line_number & 0b111) * 4;
			break;

		default:
			throw internal_error();
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

} // namespace genesis::vdp::impl

#endif // __VDP_IMPL_HSCROLL_TABLE_H__
