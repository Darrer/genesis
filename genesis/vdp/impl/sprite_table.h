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


class sprite_table
{
public:
	sprite_table(genesis::vdp::settings& sett, genesis::vdp::vram_t& vram)
		: sett(sett), vram(vram)
	{
	}

	unsigned num_entries() const
	{
		return sett.display_width() == display_width::c32 ? 64 : 80;
	}

	// entry is zero-based
	sprite_table_entry get(unsigned entry_number) const
	{
		if(entry_number >= num_entries())
			throw std::invalid_argument("entry_number");

		const std::uint32_t entry_size = 8;
		std::uint32_t address = sett.sprite_address() + (entry_number * entry_size);

		sprite_table_entry entry;

		// Read vertical position
		std::uint8_t vp2 = vram.read<std::uint8_t>(address++) & 0b1; // TODO: or 0b11?
		std::uint8_t vp1 = vram.read<std::uint8_t>(address++);
		entry.vertical_position = (vp2 << 8) | vp1;

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
		std::uint8_t hp2 = vram.read<std::uint8_t>(address++) & 0b1;
		std::uint8_t hp1 = vram.read<std::uint8_t>(address++);
		entry.horizontal_position = (hp2 << 8) | hp1;

		return entry;
	}

private:
	genesis::vdp::settings& sett;
	genesis::vdp::vram_t& vram;
};

}

#endif // __VDP_IMPL_SPRITE_TABLE_H__
