#ifndef __VDP_IMPL_SPRITE_TABLE_H__
#define __VDP_IMPL_SPRITE_TABLE_H__

#include <cstdint>
#include <stdexcept>

#include "vdp/memory.h"
#include "vdp/settings.h"

namespace genesis::vdp::impl
{

struct sprite_table_entry
{
	std::uint16_t vertical_position;
	std::uint16_t horizontal_position;

	std::uint8_t vertical_size;
	std::uint8_t horizontal_size;

	bool vertical_flip;
	bool horizontal_flip;

	std::uint32_t pattern_address;
	std::uint8_t palette;
	std::uint8_t link;
	bool priority_flag;
};

static_assert(sizeof(sprite_table_entry) == 16);


class sprite_table
{
public:
	sprite_table(genesis::vdp::settings& sett, genesis::vdp::vram_t& vram)
		: vram(vram)
	{
		m_num_entries = num_entries_impl(sett);
		m_sprite_address = sett.sprite_address();
	}

	int num_entries() const
	{
		return m_num_entries;
	}

	// entry is zero-based
	sprite_table_entry get(int entry_number) const
	{
		if(entry_number >= num_entries())
			throw std::invalid_argument("entry_number");

		std::uint32_t address = m_sprite_address + (entry_number * 8 /* entry size */);

		sprite_table_entry entry;

		// Read vertical position
		entry.vertical_position = vram.read<std::uint16_t>(address) & 0b111'111'111; // use 9 bits only
		address += 2;

		// Read sprite size
		std::uint8_t size = vram.read<std::uint8_t>(address++);
		entry.vertical_size = size & 0b11;
		entry.horizontal_size = (size >> 2) & 0b11;

		// Read link
		std::uint8_t link = vram.read<std::uint8_t>(address++);
		entry.link = link & 0b1111111;

		// Read pattern address
		std::uint8_t pattern_addr2 = vram.read<std::uint8_t>(address++);
		std::uint8_t pattern_addr = vram.read<std::uint8_t>(address++);
		entry.pattern_address = pattern_addr | ((pattern_addr2 & 0b111) << 8);
		entry.pattern_address = entry.pattern_address << 5;

		// These fields are part of pattern address 2
		entry.horizontal_flip = (pattern_addr2 >> 3) & 0b1;
		entry.vertical_flip = (pattern_addr2 >> 4) & 0b1;
		entry.palette = (pattern_addr2 >> 5) & 0b11;
		entry.priority_flag = (pattern_addr2 >> 7) & 0b1;

		// Read horizontal position
		entry.horizontal_position = vram.read<std::uint16_t>(address) & 0b111111111; // use 9 bits only

		return entry;
	}

private:
	static int num_entries_impl(vdp::settings& sett)
	{
		return sett.display_width() == display_width::c32 ? 64 : 80;
	}

private:
	genesis::vdp::vram_t& vram;

	int m_num_entries;
	std::uint32_t m_sprite_address;
};

}

#endif // __VDP_IMPL_SPRITE_TABLE_H__
