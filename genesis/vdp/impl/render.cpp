#include "render.h"

#include <iostream>

namespace genesis::vdp::impl
{





const auto PIXELS_IN_TILE_ROW = 8;
const auto PIXELS_IN_TILE_COL = 8;

// simplify for now
const auto TILE_SIZE_IN_BYTES = 32; // 8x8 pixels * 4 bits per pixel

render::render(genesis::vdp::register_set& regs, genesis::vdp::settings& sett,
	genesis::vdp::vram_t& vram,  genesis::vdp::vsram_t& vsram, genesis::vdp::cram_t& cram):
	regs(regs), sett(sett), vram(vram), vsram(vsram), cram(cram)
{
}

std::uint16_t render::background_color() const
{
	return cram.read_color(regs.R7.PAL, regs.R7.COL);
}

std::span<genesis::vdp::color> render::get_plane_b_row(std::uint8_t row_number)
{
	name_table table(plane_type::b, sett, vram);
	auto tiles_per_row = sett.plane_width_in_tiles();
	const int tile_row_number = row_number / PIXELS_IN_TILE_COL;
	const int row_in_tail = row_number % PIXELS_IN_TILE_COL;

	int buffer_idx = 0;
	for(auto i = 0; i < table.entries_per_row(); ++i)
	{
		auto entry = table.get(tile_row_number, i);

		// TODO: does this code perform LE/BE convertion?
		std::uint32_t tail = read_tail_row(row_in_tail, entry);

		// there are always 8 row pixels in tail
		for(int i = 0; i < 8; ++i)
		{
			std::uint8_t color_idx = tail & 0b1111;
			tail = tail >> 4;

			plane_buffer.at(buffer_idx++) = cram.read_color(entry.palette, color_idx);
		}
	}

	return std::span<genesis::vdp::color>(plane_buffer.begin(), tiles_per_row * PIXELS_IN_TILE_ROW);
}

// row_number - zero based
std::uint32_t render::read_tail_row(std::uint8_t row_number, name_table_entry entry) const
{
	const int row_size = 4;
	std::uint32_t address = entry.effective_pattern_address() + (row_number * row_size);
	auto row = vram.read<std::uint32_t>(address);

	if(entry.horizontal_flip)
	{
		std::uint32_t out_row = 0;
		for(int i = 0; i < 8; ++i)
		{
			std::uint8_t value = row & 0xF;
			row = row >> 4;

			out_row |= value << (28 - (i * 4));
		}

		row = out_row;
	}

	return row;
}

}
