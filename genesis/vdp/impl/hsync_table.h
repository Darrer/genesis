#ifndef __VDP_IMPL_HSYNC_TABLE_H__
#define __VDP_IMPL_HSYNC_TABLE_H__

#include <cstdint>
#include <stdexcept>

#include "vdp/memory.h"
#include "vdp/settings.h"
#include "plane_type.h"


namespace genesis::vdp::impl
{

struct hsync_table_entry
{
public:
	hsync_table_entry(std::uint16_t raw_value)
	{
		pixel_offset = raw_value & 0b111;
		tail_offset = (raw_value >> 3) & 0b1111111;
	}

	const std::uint8_t pixel_offset;
	const std::uint8_t tail_offset;
};


// horizontal scroll table
class hsync_table
{
public:
	hsync_table(plane_type plane, genesis::vdp::settings& sett, genesis::vdp::vram_t& vram)
		: plane(plane), sett(sett), vram(vram)
	{
		if(plane != plane_type::a && plane != plane_type::b)
			throw std::invalid_argument("plane");
	}

	/* unsigned num_entries() const
	{
		switch (sett.horizontal_scrolling())
		{
		case horizontal_scrolling::full_screen:
			return 1; // there are only 1 element

		case horizontal_scrolling::invalid: // undocumented behavior
			return 8; // or return 240/224?

		case horizontal_scrolling::line:
			if(sett.display_height() == display_height::c30)
				return 240;
			return 224;

		case horizontal_scrolling::cell:
			if(sett.display_height() == display_height::c30)
				return 30;
			return 28;

		default: throw internal_error();
		}
	} */

	hsync_table_entry get(unsigned line_number) const
	{
		if(line_number >= max_lines())
			throw std::invalid_argument("line_number");

		return vram.read<std::uint16_t>(get_address(line_number));
	}

private:
	std::uint32_t get_address(unsigned line_number) const
	{
		std::uint32_t address = sett.horizontal_scroll_address();
		switch (sett.horizontal_scrolling())
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
			// TODO: not checked
			address += (line_number & ~0b111) * 32;
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

	unsigned max_lines() const
	{
		switch (sett.display_height())
		{
		case display_height::c30:
			return 240;
		case display_height::c28:
			return 224;
		default: throw internal_error();
		}
	}

private:
	plane_type plane;
	genesis::vdp::settings& sett;
	genesis::vdp::vram_t& vram;
};

}

#endif // __VDP_IMPL_HSYNC_TABLE_H__
