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

		auto row = read_pattern_row(row_in_tail, entry.effective_pattern_address(), entry.horizontal_flip, entry.vertical_flip);
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

std::span<genesis::vdp::output_color> render::get_sprite_row(unsigned row_number,
	std::span<genesis::vdp::output_color> buffer) const
{
	if(buffer.size() < sprite_width_in_pixels())
		throw genesis::internal_error();

	std::fill_n(buffer.begin(), sprite_width_in_pixels(), TRANSPARENT_COLOR);

	sprite_table stable(sett, vram);

	unsigned sprite_number = 0;
	// TODO: introduce limits per scanline
	for(unsigned i = 0; i < stable.num_entries(); ++i)
	{
		auto entry = stable.get(sprite_number);

		if(should_draw_sprite(row_number, entry))
		{
			draw_sprite(row_number, entry, buffer);
		}

		sprite_number = entry.link;
		if(sprite_number == 0)
			break;
	}

	return std::span<genesis::vdp::output_color>(buffer.begin(), sprite_width_in_pixels());
}

// pattern_row_number - zero based
std::array<std::uint8_t, 4> render::read_pattern_row(unsigned pattern_row_number, std::uint32_t pattern_addres,
	bool hflip, bool vflip) const
{
	// TODO: can it be > 8?
	if(pattern_row_number > 8)
		throw internal_error();

	if(vflip)
		pattern_row_number = 7 - pattern_row_number;

	pattern_addres = pattern_addres + (pattern_row_number * 0x4 /* single row occupies 4 bytes */);

	std::array<std::uint8_t, 4> row;
	if(hflip)
	{
		for(int i = 3; i >= 0; --i)
		{
			row[i] = vram.read<std::uint8_t>(pattern_addres++);
			endian::swap_nibbles(row[i]);
		}
	}
	else
	{
		for(int i = 0; i < 4; ++i)
			row[i] = vram.read<std::uint8_t>(pattern_addres++);
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

bool render::should_draw_sprite(unsigned row_number, const sprite_table_entry& entry) const
{
	// TODO: assume pattern is always 8 pixels height
	std::uint16_t last_vert_pos = entry.vertical_position + ((entry.vertical_size + 1) * 8);

	return entry.vertical_position <= row_number && row_number < last_vert_pos;
}

void render::draw_sprite(unsigned row_number, const sprite_table_entry& entry,
	std::span<genesis::vdp::output_color> buffer) const
{
	auto it = buffer.begin();
	std::advance(it, entry.horizontal_position);

	unsigned pattern_row_number = (row_number - entry.vertical_position) % 8;

	for(int i = 0; i <= entry.horizontal_size; ++i)
	{
		std::uint32_t pattern_addr = sprite_pattern_address(row_number, i, entry);
		auto row = read_pattern_row(pattern_row_number, pattern_addr, entry.horizontal_flip, entry.vertical_flip);

		// dump
		for(std::uint8_t color : row)
		{
			// every element contains 2 colors
			std::uint8_t first_idx = color >> 4;
			std::uint8_t second_idx = color & 0xF;

			auto first_color = read_color(entry.palette, first_idx);
			auto second_color = read_color(entry.palette, second_idx);

			if(*it == TRANSPARENT_COLOR)
				*it = first_color;
			it++;

			if(*it == TRANSPARENT_COLOR)
				*it = second_color;
			it++;
		}
	}
}

// column number is from left to right in range [0; 3]
std::uint32_t render::sprite_pattern_address(unsigned row_number, unsigned sprite_column_number, 
	const sprite_table_entry& entry) const
{
	if(entry.horizontal_flip)
		sprite_column_number = entry.horizontal_size - sprite_column_number;

	// TODO: tmp check
	if(sprite_column_number > 3)
	{
		std::cout << "Column number: " << sprite_column_number << '\n';
		throw genesis::internal_error();
	}
	
	// convert pattern number
	unsigned sprite_row = (row_number - entry.vertical_position) / 8;
	if(entry.vertical_flip)
		sprite_row = entry.vertical_size - sprite_row;

	// TODO: tmp check
	if(sprite_row > 3)
	{
		std::cout << "Sprite row: " << row_number << " - " << entry.vertical_position << "\n";
		throw genesis::internal_error();
	}

	unsigned sprite_pattern_number = sprite_row + (sprite_column_number * (entry.vertical_size + 1));

	// TODO: tmp check
	if(sprite_pattern_number > 0xF)
		throw genesis::internal_error();

	std::uint32_t address = entry.pattern_address + (sprite_pattern_number * 32); // each pattern is 32 bytes
	return address;
}

}
