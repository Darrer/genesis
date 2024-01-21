#ifndef __VDP_IMPL_NAME_TABLE_H__
#define __VDP_IMPL_NAME_TABLE_H__

#include <cstdint>
#include <stdexcept>

#include "vdp/memory.h"
#include "vdp/settings.h"
#include "plane_type.h"

namespace genesis::vdp::impl
{

struct name_table_entry
{
	name_table_entry() : name_table_entry(0) { }

	name_table_entry(std::uint16_t value)
	{
		// TODO: check if it's UB
		*reinterpret_cast<std::uint16_t*>(this) = value;
	}

	std::uint32_t effective_pattern_address() const
	{
		return pattern_addr << 5;
	}

	std::uint16_t pattern_addr : 11;
	std::uint16_t horizontal_flip : 1;
	std::uint16_t vertical_flip : 1;
	std::uint16_t palette : 2;
	std::uint16_t priority : 1;
};

static_assert(sizeof(name_table_entry) == 2);

class name_table
{
public:
	name_table(plane_type plane, genesis::vdp::settings& sett, genesis::vdp::vram_t& vram)
		: vram(vram)
	{
		m_plane_address = plane_address_impl(plane, sett);
		m_entries_per_row = entries_per_row_impl(plane, sett);
		m_row_count = row_count_impl(plane, sett);
		m_row_size_in_bytes = m_entries_per_row * sizeof(name_table_entry);
	}

	int entries_per_row() const
	{
		return m_entries_per_row;
	}

	int row_count() const
	{
		return m_row_count;
	}

	// row_number & entry_number are zero-based
	name_table_entry get(int row_number, int entry_number) const
	{
		if(row_number >= row_count())
			throw std::invalid_argument("row_number");
		
		if(entry_number >= entries_per_row())
			throw std::invalid_argument("entry_number");

		std::uint32_t address = m_plane_address
			+ (m_row_size_in_bytes * row_number)
			+ (entry_number * sizeof(name_table_entry));

		return vram.read<std::uint16_t>(address);
	}

private:
	static std::uint32_t plane_address_impl(plane_type plane, vdp::settings& sett)
	{
		switch (plane)
		{
		case plane_type::a:
			return sett.plane_a_address();
		case plane_type::b:
			return sett.plane_b_address();
		case plane_type::w:
			return sett.plane_w_address();
		default: throw internal_error();
		}
	}

	static int entries_per_row_impl(plane_type plane, vdp::settings& sett)
	{
		switch (plane)
		{
		case plane_type::a:
		case plane_type::b:
			// single table element represent single tile
			return sett.plane_width_in_tiles();
		case plane_type::w:
			if(sett.display_width() == display_width::c40)
				return 64;
			return 32;
		default: throw internal_error();
		}
	}

	static int row_count_impl(plane_type plane, vdp::settings& sett)
	{
		switch (plane)
		{
		case plane_type::a:
		case plane_type::b:
			return sett.plane_height_in_tiles();
		case plane_type::w:
			return 32; // there are alwyas 32 rows for window plane
		default: throw internal_error();
		}
	}

private:
	genesis::vdp::vram_t& vram;

	std::uint32_t m_plane_address;
	int m_entries_per_row;
	int m_row_count;
	int m_row_size_in_bytes;
};

};

#endif // __VDP_IMPL_NAME_TABLE_H__
