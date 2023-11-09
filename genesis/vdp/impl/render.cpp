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

std::span<genesis::vdp::output_color> render::get_plane_b_row(std::uint8_t row_number,
	std::span<genesis::vdp::output_color> buffer) const
{
	std::size_t min_buffer_size = sett.plane_width_in_tiles() * 8;
	if(buffer.size() < min_buffer_size)
		throw genesis::internal_error();

	name_table table(plane_type::b, sett, vram);
	const int tile_row_number = row_number / PIXELS_IN_TILE_COL;
	const int row_in_tail = row_number % PIXELS_IN_TILE_COL;

	auto it = buffer.begin();
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

			*it = read_color(entry.palette, color_idx);
			++it;
		}
	}

	return std::span<genesis::vdp::output_color>(buffer.begin(), min_buffer_size);
}

// row_number - zero based
std::uint32_t render::read_tail_row(std::uint8_t row_number, name_table_entry entry) const
{
	// TODO: can it be > 8?
	if(row_number > 8)
		throw internal_error();

	if(entry.vertical_flip)
		row_number = 7 - row_number;

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

// shouldn't be used for background color
vdp::output_color render::read_color(std::uint8_t palette_idx, std::uint8_t color_idx) const
{
	if(color_idx == 0)
		return vdp::TRANSPARENT_COLOR;
	return cram.read_color(palette_idx, color_idx);
}

}
