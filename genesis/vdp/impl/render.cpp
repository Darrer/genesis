#include "render.h"

#include "sprites_limits_tracker.h"
#include "hscroll_table.h"
#include "vscroll_table.h"

#include <algorithm>
#include <iostream>

namespace genesis::vdp::impl
{

const auto PIXELS_IN_TAILE_COL = 8;

render::render(genesis::vdp::register_set& regs, genesis::vdp::settings& sett,
	genesis::vdp::vram_t& vram,  genesis::vdp::vsram_t& vsram, genesis::vdp::cram_t& cram):
	regs(regs), sett(sett), vram(vram), vsram(vsram), cram(cram)
{
}

output_color render::background_color() const
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

unsigned render::active_display_width() const
{
	return sett.display_width_in_pixels();
}

unsigned render::active_display_height() const
{
	return sett.display_height_in_pixels();
}

std::span<genesis::vdp::output_color> render::get_plane_row(impl::plane_type plane_type,
		unsigned row_number, std::span<genesis::vdp::output_color> buffer) const
{
	std::size_t buffer_size = plane_width_in_pixels(plane_type);
	check_buffer_size(buffer, buffer_size);

	if(row_number >= plane_hight_in_pixels(plane_type))
		throw genesis::internal_error();

	name_table table(plane_type, sett, vram);
	const auto tail_row_number = row_number / PIXELS_IN_TAILE_COL;
	const auto pattern_row = row_number % PIXELS_IN_TAILE_COL;

	auto buffer_it = buffer.begin();
	for(int i = 0; i < table.entries_per_row(); ++i)
	{
		auto entry = table.get(tail_row_number, i);

		auto row = read_pattern_row(pattern_row, entry.effective_pattern_address(),
			entry.horizontal_flip, entry.vertical_flip, entry.palette);
		buffer_it = std::copy(row.begin(), row.end(), buffer_it);
	}

	return std::span<genesis::vdp::output_color>(buffer.begin(), buffer_size);
}

std::span<genesis::vdp::output_color> render::get_sprite_row(unsigned row_number,
	std::span<genesis::vdp::output_color> buffer) const
{
	const auto buffer_size = sprite_width_in_pixels();
	check_buffer_size(buffer, buffer_size);

	buffer = std::span<vdp::output_color>(buffer.begin(), buffer_size);
	std::fill(buffer.begin(), buffer.end(), TRANSPARENT_COLOR);

	sprite_table stable(sett, vram);

	int sprite_number = 0;
	for(int i = 0; i < stable.num_entries(); ++i)
	{
		auto entry = stable.get(sprite_number);

		if(should_read_sprite(row_number, entry.vertical_position, entry.vertical_size))
		{
			read_sprite(row_number, entry, buffer);
		}

		sprite_number = entry.link;
		if(sprite_number == 0 || sprite_number >= stable.num_entries())
			break;
	}

	return buffer;
}

std::span<genesis::vdp::output_color> render::get_active_display_row(unsigned row_number,
	std::span<genesis::vdp::output_color> buffer)
{
	const auto buffer_size = active_display_width();
	check_buffer_size(buffer, buffer_size);

	buffer = std::span<genesis::vdp::output_color>(buffer.begin(), buffer_size);

	auto plane_a = get_active_plane_row(plane_type::a, row_number, pixel_a_buffer);
	auto plane_b = get_active_plane_row(plane_type::b, row_number, pixel_b_buffer);
	auto window = get_active_window_row(row_number, window_buffer);
	auto sprites = get_active_sprites_row(row_number, sprite_buffer);

	// if(buffer_size != plane_a.size() || buffer_size != plane_b.size()
	// 	|| buffer_size != sprites.size() || buffer_size != window.size())
	// 	throw genesis::internal_error();

	vdp::output_color bg_color = background_color();

	for(std::size_t i = 0; i < buffer.size(); ++i)
	{
		buffer[i] = resolve_priority(bg_color, plane_a[i], plane_b[i], window[i], sprites[i]);
		// buffer[i] = resolve_priority(bg_color, plane_a[i], plane_b[i], TRANSPARENT_PIXEL, sprites[i]);
	}

	return buffer;
}

void render::reset_limits()
{
}

std::span<render::pixel> render::get_active_plane_row(plane_type plane_type, unsigned row_number,
	std::span<render::pixel> buffer) const
{
	std::size_t buffer_size = active_display_width();
	check_buffer_size(buffer, buffer_size);

	if(row_number >= active_display_height())
		throw genesis::internal_error();

	auto plane = get_scrolled_plane_row(plane_type, row_number, buffer);

	// TODO: what if returned plane size is less then active display row?
	if(plane.size() < buffer_size)
		throw genesis::not_implemented();

	return std::span<render::pixel>(buffer.begin(), buffer_size);
}

std::span<render::pixel> render::get_scrolled_plane_row(impl::plane_type plane_type,
	unsigned row_number, std::span<render::pixel> buffer) const
{
	std::size_t buffer_size = plane_width_in_pixels(plane_type);
	check_buffer_size(buffer, buffer_size);

	buffer = std::span<pixel>(buffer.begin(), buffer_size);

	unsigned max_height = plane_hight_in_pixels(plane_type);

	name_table table(plane_type, sett, vram);

	auto buffer_it = buffer.begin();
	for(int tail_column_number = 0; tail_column_number < table.entries_per_row(); ++tail_column_number)
	{
		// apply horizontal scrolling just by changing row_number
		int offset = vscroll_table::get_offset(plane_type, tail_column_number, sett, vsram);
		int shifted_row_number = (row_number + offset) % max_height;

		int tail_row_number = shifted_row_number / PIXELS_IN_TAILE_COL;
		int tail_row = shifted_row_number % PIXELS_IN_TAILE_COL;

		name_table_entry entry = table.get(tail_row_number, tail_column_number);

		auto pattern_row = read_pattern_row(tail_row, entry.effective_pattern_address(),
			entry.horizontal_flip, entry.vertical_flip, entry.palette);
		bool priority = entry.priority == 1;

		buffer_it = std::transform(pattern_row.begin(), pattern_row.end(), buffer_it,
			[priority](output_color color) -> pixel { return { color, priority }; });
	}

	// apply horizontal scrolling
	hscroll_table hscroll(plane_type, sett, vram);
	int offset = hscroll.get_offset(row_number) % plane_width_in_pixels(plane_type);
	std::rotate(buffer.rbegin(), buffer.rbegin() + offset, buffer.rend());

	return buffer;
}

std::span<render::pixel> render::get_active_window_row(unsigned line_number, std::span<render::pixel> buffer) const
{
	std::size_t buffer_size = active_display_width();
	check_buffer_size(buffer, buffer_size);

	buffer = std::span<pixel>(buffer.begin(), buffer_size);
	std::fill(buffer.begin(), buffer.end(), TRANSPARENT_PIXEL);

	// range: [start_col; end_col), 0-based, in tails
	int start_col = regs.R17.R == 0 ? 0 : (regs.R17.HP * 2);
	int end_col = regs.R17.R == 0 ? (regs.R17.HP * 2) : sett.display_width_in_tails();

	// range: [start_row; end_row), 0-based, in tails
	int start_row = regs.R18.D == 0 ? 0 : regs.R18.VP;
	int end_row = regs.R18.D == 0 ? regs.R18.VP : sett.display_height_in_tails();

	/* NOTE:
	 * Some games use R17.R = 0 and R17.HP = 0 to display a full-width window.
	 * However, I can't find that this behavior is actually documented.
	 * So emulate this behavior based on empirical observations.
	 * 
	 * Perhaps the same rule applies to the R18 register, but it's very unlikely
	 * that any games actually use the full-height window plane.
	 */
	if(regs.R17.R == 0 && regs.R17.HP == 0)
		end_col = sett.display_width_in_tails();

	int tail_row = line_number / 8;
	// if(tail_row == 0)
	// {
	// 	std::cout << "Window settings: R: " << (int)regs.R17.R << ", HP: " << (int)regs.R17.HP
	// 		<< ", D: " << (int)regs.R18.D << ", VP: " << (int)regs.R18.VP << "\n";
	// 	std::cout << "Window start row: " << start_row << ", end row: " << end_row << ", start col: " << start_col << ", end col: " << end_col << "\n";
	// }

	if(tail_row < start_row || tail_row >= end_row)
		return buffer;

	auto buffer_it = buffer.begin();
	name_table table(plane_type::w, sett, vram);
	for(int col = start_col; col < end_col; ++col)
	{
		name_table_entry entry = table.get(tail_row, col);

		auto pattern_row = read_pattern_row(line_number % 8, entry.effective_pattern_address(),
			entry.horizontal_flip, entry.vertical_flip, entry.palette);

		buffer_it = std::transform(pattern_row.begin(), pattern_row.end(), buffer_it,
			[&entry](auto color) -> pixel { return { color, true }; });
	}

	return buffer;
}

// Rename to render_active_sprite_line
std::span<render::pixel> render::get_active_sprites_row(unsigned row_number, std::span<render::pixel> buffer)
{
	const auto buffer_size = sprite_width_in_pixels();
	check_buffer_size(buffer, buffer_size);

	bool prev_line_overflow = regs.SR.SO == 1;
	regs.SR.SO = 0;
	regs.SR.SC = 0;

	row_number += 128;

	buffer = std::span<pixel>(buffer.begin(), buffer_size);
	std::fill(buffer.begin(), buffer.end(), TRANSPARENT_PIXEL);

	sprites_limits_tracker sprites_limits(sett);
	sprite_table stable(sett, vram);

	int sprite_number = 0;
	unsigned rendered_sprites = 0;
	bool masked = false;
	for(int i = 0; i < stable.num_entries(); ++i)
	{
		// check limits
		if(sprites_limits.line_limit_exceeded())
		{
			regs.SR.SO = 1;
			break;
		}

		auto entry = stable.get(sprite_number);

		if(should_read_sprite(row_number, entry.vertical_position, entry.vertical_size))
		{
			// Sprite masking
			if(entry.horizontal_position == 0 && (rendered_sprites > 0 || prev_line_overflow))
			{
				// All other sprites should be masked
				masked = true;
			}

			if(entry.horizontal_position != 0)
				++rendered_sprites;

			if(masked == false)
			{
				bool collision = read_sprite(row_number, entry, buffer, sprites_limits.line_pixels_limit());
				if(collision)
					regs.SR.SC = 1;
			}

			// Keep tracking limits to correctly set Sprite Overflow (SO) flag
			sprites_limits.on_sprite_draw(entry);
		}

		sprite_number = entry.link;
		if(sprite_number == 0 || sprite_number >= stable.num_entries())
			break;
	}

	// sprites on active display starts on 128 position
	auto first_it = std::next(buffer.begin(), 128);
	return std::span<pixel>(first_it, active_display_width());
}

genesis::vdp::output_color render::resolve_priority(genesis::vdp::output_color background_color,
	pixel plane_a, pixel plane_b, pixel window, pixel sprite) const
{
	if(window.color != TRANSPARENT_COLOR && window.priority_flag)
		return window.color;
	if(sprite.color != TRANSPARENT_COLOR && sprite.priority_flag)
		return sprite.color;
	if(plane_a.color != TRANSPARENT_COLOR && plane_a.priority_flag)
		return plane_a.color;
	if(plane_b.color != TRANSPARENT_COLOR && plane_b.priority_flag)
		return plane_b.color;

	if(window.color != TRANSPARENT_COLOR)
		return window.color;
	if(sprite.color != TRANSPARENT_COLOR)
		return sprite.color;
	if(plane_a.color != TRANSPARENT_COLOR)
		return plane_a.color;
	if(plane_b.color != TRANSPARENT_COLOR)
		return plane_b.color;

	return background_color;
}

// pattern_row_number - zero based
std::array<vdp::output_color, 8> render::read_pattern_row(unsigned pattern_row_number, std::uint32_t pattern_addres,
	bool hflip, bool vflip, std::uint8_t palette_id) const
{
	if(pattern_row_number >= 8)
		throw internal_error();

	if(vflip)
		pattern_row_number = 7 - pattern_row_number;

	pattern_addres = pattern_addres + (pattern_row_number * 0x4 /* single row occupies 4 bytes */);

	std::uint32_t row_data = vram.read_raw<std::uint32_t>(pattern_addres);

	std::array<vdp::output_color, 8> colors;

	if(hflip)
	{
		for(auto it = colors.rbegin(); it != colors.rend();)
		{
			unsigned second_idx = row_data & 0xF;
			row_data = row_data >> 4;

			unsigned first_idx = row_data & 0xF;
			row_data = row_data >> 4;

			*(it++) = read_color(palette_id, first_idx);
			*(it++) = read_color(palette_id, second_idx);
		}
	}
	else
	{
		for(auto it = colors.begin(); it != colors.end();)
		{
			unsigned second_idx = row_data & 0xF;
			row_data = row_data >> 4;

			unsigned first_idx = row_data & 0xF;
			row_data = row_data >> 4;

			*(it++) = read_color(palette_id, first_idx);
			*(it++) = read_color(palette_id, second_idx);
		}
	}

	return colors;
}

// shouldn't be used for background color
vdp::output_color render::read_color(unsigned palette_idx, unsigned color_idx) const
{
	if(color_idx == 0)
		return vdp::TRANSPARENT_COLOR;
	return cram.read_color(palette_idx, color_idx);
}

// TODO: rename to should_render_sprite
bool render::should_read_sprite(unsigned row_number, unsigned vertical_position, unsigned vertical_size) const
{
	unsigned last_vert_pos = vertical_position + ((vertical_size + 1) * 8);
	return vertical_position <= row_number && row_number < last_vert_pos;
}

void render::read_sprite(unsigned row_number, const sprite_table_entry& entry,
	std::span<vdp::output_color> dest) const
{
	check_buffer_size(dest, entry.horizontal_position);

	auto dest_it = std::next(dest.begin(), entry.horizontal_position);

	unsigned pattern_row_number = (row_number - entry.vertical_position) % 8;

	for(int i = 0; i <= entry.horizontal_size; ++i)
	{
		std::uint32_t pattern_addr = sprite_pattern_address(row_number, i, entry);
		auto row = read_pattern_row(pattern_row_number, pattern_addr,
			entry.horizontal_flip, entry.vertical_flip, entry.palette);
		for(auto color : row)
		{
			if(*dest_it == TRANSPARENT_COLOR)
				*dest_it = color;

			if(++dest_it == dest.end())
				return;
		}
	}
}

bool render::read_sprite(unsigned row_number, const sprite_table_entry& entry,
	std::span<render::pixel> dest, unsigned pixels_limit) const
{
	check_buffer_size(dest, entry.horizontal_position);

	auto dest_it = std::next(dest.begin(), entry.horizontal_position);

	unsigned pattern_row_number = (row_number - entry.vertical_position) % 8;

	bool collision = false;

	if(pixels_limit == 0)
		return collision;

	for(int i = 0; i <= entry.horizontal_size; ++i)
	{
		std::uint32_t pattern_addr = sprite_pattern_address(row_number, i, entry);
		auto row = read_pattern_row(pattern_row_number, pattern_addr,
			entry.horizontal_flip, entry.vertical_flip, entry.palette);
		for(auto color : row)
		{
			if(dest_it->color == TRANSPARENT_COLOR)
			{
				*dest_it = { color, entry.priority_flag };
			}
			else if(color != TRANSPARENT_COLOR)
			{
				collision = true;
			}

			// TODO: does pixels limit affect collision bit?
			--pixels_limit;
			if(++dest_it == dest.end() || pixels_limit == 0)
				return collision;
		}
	}

	return collision;
}

// column number is from left to right in range [0; 3]
std::uint32_t render::sprite_pattern_address(unsigned row_number, unsigned sprite_column_number, 
	const sprite_table_entry& entry) const
{
	if(entry.horizontal_flip)
		sprite_column_number = entry.horizontal_size - sprite_column_number;
	
	// convert pattern number
	unsigned sprite_row = (row_number - entry.vertical_position) / 8;
	if(entry.vertical_flip)
		sprite_row = entry.vertical_size - sprite_row;

	unsigned sprite_pattern_number = sprite_row + (sprite_column_number * (entry.vertical_size + 1));

	std::uint32_t address = entry.pattern_address + (sprite_pattern_number * 32); // each pattern is 32 bytes
	return address;
}

}
