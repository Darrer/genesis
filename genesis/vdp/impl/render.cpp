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

unsigned render::plane_width_in_pixels(plane_type plane_type) const
{
	name_table table(plane_type, sett, vram);
	// as each entry represent 1 tail which is 8 pixeles width
	return table.entries_per_row() * 8;
}

unsigned render::plane_hight_in_pixels(plane_type plane_type) const
{
	name_table table(plane_type, sett, vram);
	// as each row is 1 tail hight
	return table.row_count() * 8;
}

std::span<genesis::vdp::output_color> render::get_plane_row(impl::plane_type plane_type,
		unsigned row_number, std::span<genesis::vdp::output_color> buffer) const
{
	std::size_t min_buffer_size = plane_width_in_pixels(plane_type);
	if(buffer.size() < min_buffer_size)
		throw genesis::internal_error();

	unsigned max_rows = plane_hight_in_pixels(plane_type);
	if(row_number >= max_rows)
		throw genesis::internal_error();

	name_table table(plane_type, sett, vram);
	const auto tile_row_number = row_number / PIXELS_IN_TILE_COL;
	const auto row_in_tail = row_number % PIXELS_IN_TILE_COL;

	auto it = buffer.begin();
	for(auto i = 0; i < table.entries_per_row(); ++i)
	{
		auto entry = table.get(tile_row_number, i);

		auto row = read_tail_row(row_in_tail, entry);
		for(auto i : row)
		{
			// every element contains 2 colors
			std::uint8_t first_idx = i >> 4;
			std::uint8_t second_idx = i & 0xF;

			*it = read_color(entry.palette, first_idx);
			++it;

			*it = read_color(entry.palette, second_idx);
			++it;
		}
	}

	return std::span<genesis::vdp::output_color>(buffer.begin(), min_buffer_size);
}

// row_number - zero based
std::array<std::uint8_t, 4> render::read_tail_row(unsigned row_number, name_table_entry entry) const
{
	// TODO: can it be > 8?
	if(row_number > 8)
		throw internal_error();

	if(entry.vertical_flip)
		row_number = 7 - row_number;

	const int row_size = 0x4;
	std::uint32_t address = entry.effective_pattern_address() + (row_number * row_size);

	std::array<std::uint8_t, 4> row;
	if(entry.horizontal_flip)
	{
		for(int i = 3; i >= 0; --i)
		{
			row[i] = vram.read<std::uint8_t>(address++);
			endian::swap_nibbles(row[i]);
		}
	}
	else
	{
		for(int i = 0; i < 4; ++i)
			row[i] = vram.read<std::uint8_t>(address++);
	}

	return row;
}

// shouldn't be used for background color
vdp::output_color render::read_color(unsigned palette_idx, unsigned color_idx) const
{
	if(color_idx == 0)
		return vdp::TRANSPARENT_COLOR;
	return cram.read_color(palette_idx, color_idx);
}

}
