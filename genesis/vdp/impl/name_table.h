#ifndef __VDP_IMPL_NAME_TABLE_H__
#define __VDP_IMPL_NAME_TABLE_H__

#include <cstdint>

#include "vdp/memory.h"
#include "vdp/settings.h"

namespace genesis::vdp::impl
{

union name_table_entry
{
	name_table_entry() : raw_value(0) { }
	name_table_entry(std::uint16_t value) : raw_value (value) { }

	std::uint32_t effective_pattern_address() const
	{
		return pattern_addr << 5;
	}

	struct 
	{
		std::uint16_t pattern_addr : 11;
		std::uint16_t horizontal_flip : 1;
		std::uint16_t vertical_flip : 1;
		std::uint16_t palette : 2;
		std::uint16_t priority : 1;
	};

	std::uint16_t raw_value;
};

static_assert(sizeof(name_table_entry) == 2);

enum class plane_type
{
	a,
	b
};

class name_table
{
public:
	name_table(plane_type plane, genesis::vdp::settings& sett, genesis::vdp::vram_t& vram)
		: plane(plane), sett(sett), vram(vram)
	{
	}

	std::uint8_t entries_per_row() const
	{
		// single table element represent single tile
		return sett.plane_width_in_tiles();
	}

	std::uint8_t rows() const
	{
		return sett.plane_height_in_tiles();
	}

	name_table_entry get(std::uint8_t row_number, std::uint8_t entry_number) const
	{
		const int row_size_in_bytes = entries_per_row() * sizeof(name_table_entry);

		std::uint32_t address = plane_address()
			+ (row_size_in_bytes * row_number)
			+ (entry_number * sizeof(name_table_entry));
		
		return vram.read<std::uint16_t>(address);
	}

private:
	std::uint32_t plane_address() const
	{
		switch (plane)
		{
		case plane_type::a:
			return sett.plane_a_address();
		case plane_type::b:
			return sett.plane_b_address();
		default: throw internal_error();
		}
	}

private:
	plane_type plane;
	genesis::vdp::settings& sett;
	genesis::vdp::vram_t& vram;
};

};

#endif // __VDP_IMPL_NAME_TABLE_H__
