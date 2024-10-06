#include "render.h"

#include "sprites_limits_tracker.h"
#include "hscroll_table.h"
#include "vscroll_table.h"

#include <algorithm>
#include <cassert>
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

unsigned render::plane_height_in_pixels(plane_type plane_type) const
{
	name_table table(plane_type, sett, vram);
	// as each row is 1 tail height
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

	if(row_number >= plane_height_in_pixels(plane_type))
		throw std::invalid_argument("provided invalid row_number");

	name_table table(plane_type, sett, vram);
	const auto tail_row_number = row_number / PIXELS_IN_TAILE_COL;
	const auto pattern_row = row_number % PIXELS_IN_TAILE_COL;

	buffer = std::span<genesis::vdp::output_color>(buffer.begin(), buffer_size);
	auto buffer_it = buffer.begin();

	for(int i = 0; i < table.entries_per_row(); ++i)
	{
		auto entry = table.get(tail_row_number, i);

		auto on_pixel_read = [&](int color_id)
		{
			*(buffer_it++) = read_color(entry.palette, color_id);
		};

		read_pattern_line(pattern_row, entry.effective_pattern_address(),
			entry.horizontal_flip, entry.vertical_flip, on_pixel_read);
	}

	assert(buffer_it == buffer.end());
	return buffer;
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

		if(should_render_sprite(row_number, entry.vertical_position, entry.vertical_size))
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
	if(row_number >= active_display_height())
		throw std::invalid_argument("row_number exceeds active display height");

	const auto buffer_size = active_display_width();
	check_buffer_size(buffer, buffer_size);

	auto a_buffer = get_active_plane_row(plane_type::a, row_number, pixel_a_buffer);
	auto b_buffer = get_active_plane_row(plane_type::b, row_number, pixel_b_buffer);

	auto sprites = get_active_sprites_row(row_number, sprite_buffer);

	render_active_window_row(row_number, a_buffer);

	vdp::output_color bg_color = background_color();

	buffer = std::span<genesis::vdp::output_color>(buffer.begin(), buffer_size);
	for(std::size_t i = 0; i < buffer.size(); ++i)
	{
		buffer[i] = resolve_priority(bg_color, a_buffer[i], b_buffer[i], sprites[i]);
	}

	return buffer;
}

void render::reset_limits()
{
}

std::span<render::internal_pixel> render::get_active_plane_row(plane_type plane_type, unsigned row_number,
	std::span<render::internal_pixel> buffer) const
{
	const std::size_t buffer_size = active_display_width() + 8 /* room for one more tail */;
	assert(buffer_size <= buffer.size());

	unsigned max_height = plane_height_in_pixels(plane_type);

	name_table table(plane_type, sett, vram);

	hscroll_table hscroll(plane_type, sett, vram);
	int hoffset = hscroll.get_offset(row_number);

	// ceiling division by 8
	int tail_hoffset = (hoffset + 7) / 8;

	// do offset from the end
	tail_hoffset = table.entries_per_row() - (tail_hoffset % table.entries_per_row());

	auto buffer_it = buffer.begin();
	int tails_to_render = std::min((int)buffer_size / 8, table.entries_per_row());
	for(int tail = 0; tail < tails_to_render; ++tail)
	{
		// apply horizontal-tail scrolling
		int tail_column_number = (tail + tail_hoffset) & (table.entries_per_row() - 1);

		// apply vertical scrolling just by changing row_number
		int voffset = vscroll_table::get_offset(plane_type, tail_column_number, sett, vsram);
		int shifted_row_number = (row_number + voffset) & (max_height - 1);

		int tail_row_number = shifted_row_number / PIXELS_IN_TAILE_COL;
		int tail_row = shifted_row_number & (PIXELS_IN_TAILE_COL - 1);

		name_table_entry entry = table.get(tail_row_number, tail_column_number);

		auto on_pixel_read = [&](int color_id)
		{
			*(buffer_it++) = { entry.palette, color_id, entry.priority };
		};

		read_pattern_line(tail_row, entry.effective_pattern_address(),
			entry.horizontal_flip, entry.vertical_flip, on_pixel_read);
	}

	// apply horizontal-pixel scrolling
	int hpixel_offset = 0;
	if((hoffset % 8) != 0)
		hpixel_offset = 8 - (hoffset % 8);

	// sometimes plane width can be less then active display width, do the tail-based padding in this case
	if(plane_width_in_pixels(plane_type) < active_display_width())
	{
		auto begin = buffer.begin() + (hpixel_offset == 0 ? 0 : 8); // skip the first tail if needed
		auto end = buffer.begin() + hpixel_offset + active_display_width();
		// buffer_it already points to the next tail
		for(auto it = begin; buffer_it != end; ++it, ++buffer_it)
			*buffer_it = *it;
	}

	assert((std::size_t)std::distance(buffer.begin(), buffer_it) <= buffer_size);
	buffer = std::span<internal_pixel>(buffer.begin() + hpixel_offset, active_display_width());
	return buffer;
}

// render active window in plane a buffer (effectively overwriting plane a) 
void render::render_active_window_row(unsigned line_number,
	std::span<render::internal_pixel> plane_a_buffer) const
{
	const std::size_t buffer_size = active_display_width();
	assert(plane_a_buffer.size() == buffer_size);

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
		return;

	std::size_t start_position = start_col * 8;
	if(start_position >= buffer_size)
		return;

	name_table table(plane_type::w, sett, vram);

	auto buffer_it = std::next(plane_a_buffer.begin(), start_position);
	for(int col = start_col; col < end_col; ++col)
	{
		name_table_entry entry = table.get(tail_row, col);

		auto on_pixel_read = [&](int color_id)
		{
			*(buffer_it++) = { entry.palette, color_id, true };
		};

		read_pattern_line(line_number % 8, entry.effective_pattern_address(),
			entry.horizontal_flip, entry.vertical_flip, on_pixel_read);
	}

	assert(buffer_it == plane_a_buffer.end());
}

// Rename to render_active_sprite_line
std::span<render::internal_pixel> render::get_active_sprites_row(unsigned line_number, std::span<render::internal_pixel> buffer)
{
	const std::size_t buffer_size = sprite_width_in_pixels();
	assert(buffer_size <= buffer.size());

	bool prev_line_overflow = regs.SR.SO == 1;
	regs.SR.SO = 0;
	regs.SR.SC = 0;

	line_number += 128;

	buffer = std::span<internal_pixel>(buffer.begin(), buffer_size);

	internal_pixel transparent {};
	std::fill(buffer.begin(), buffer.end(), transparent);

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

		if(should_render_sprite(line_number, entry.vertical_position, entry.vertical_size))
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
				bool collision = read_sprite(line_number, entry, buffer, sprites_limits.line_pixels_limit());
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
	return std::span<internal_pixel>(first_it, active_display_width());
}

genesis::vdp::output_color render::resolve_priority(genesis::vdp::output_color background_color,
	internal_pixel plane_a, internal_pixel plane_b, internal_pixel sprite) const
{
	if(sprite.transparent())
	{
		// check only a/b
		if(plane_a.transparent())
		{
			// check only b
			if(plane_b.transparent())
				return background_color;
			return read_color(plane_b.palette_id, plane_b.color_id);
		}
		else if(plane_a.priority)
		{
			return read_color(plane_a.palette_id, plane_a.color_id);
		}
		else
		{
			if(plane_b.priority && !plane_b.transparent())
				return read_color(plane_b.palette_id, plane_b.color_id);
			return read_color(plane_a.palette_id, plane_a.color_id);
		}
	}
	else if(sprite.priority)
	{
		return read_color(sprite.palette_id, sprite.color_id);
	}
	else
	{
		// check a/b
		if(plane_a.priority && !plane_a.transparent())
			return read_color(plane_a.palette_id, plane_a.color_id);
		if(plane_b.priority && !plane_b.transparent())
			return read_color(plane_b.palette_id, plane_b.color_id);
		return read_color(sprite.palette_id, sprite.color_id);
	}
}

// shouldn't be used for background color
vdp::output_color render::read_color(unsigned palette_idx, unsigned color_idx) const
{
	if(color_idx == 0)
		return vdp::TRANSPARENT_COLOR;
	return cram.read_color(palette_idx, color_idx);
}

bool render::should_render_sprite(unsigned row_number, unsigned vertical_position, unsigned vertical_size) const
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

	bool read_more = true;
	auto on_pixel_read = [&](int color_id)
	{
		if(read_more)
		{
			if(*dest_it == TRANSPARENT_COLOR)
				*dest_it = read_color(entry.palette, color_id);
			++dest_it;

			// TODO: can we make sure we always have room for all sprites?
			if(dest_it == dest.end())
				read_more = false;
		}
	};

	for(int i = 0; read_more && i <= entry.horizontal_size; ++i)
	{
		std::uint32_t pattern_addr = sprite_pattern_address(row_number, i, entry);
		
		read_pattern_line(pattern_row_number, pattern_addr,
			entry.horizontal_flip, entry.vertical_flip, on_pixel_read);
	}

	assert((std::size_t)std::distance(dest.begin(), dest_it) <= dest.size());
}

bool render::read_sprite(unsigned row_number, const sprite_table_entry& entry,
	std::span<render::internal_pixel> dest, unsigned pixels_limit) const
{
	check_buffer_size(dest, entry.horizontal_position);

	auto dest_it = std::next(dest.begin(), entry.horizontal_position);

	unsigned pattern_row_number = (row_number - entry.vertical_position) % 8;

	bool collision = false;

	if(pixels_limit == 0)
		return collision;

	bool read_more = true;
	auto on_pixel_read = [&](int color_id)
	{
		if(read_more)
		{
			if(dest_it->transparent())
			{
				*dest_it = { entry.palette, color_id, entry.priority_flag };
			}
			else if(color_id != 0)
			{
				collision = true;
			}

			// TODO: does pixels limit affect collision bit?
			--pixels_limit;
			++dest_it;

			if(dest_it == dest.end() || pixels_limit == 0)
				read_more = false;
		}
	};

	for(int i = 0; read_more && i <= entry.horizontal_size; ++i)
	{
		std::uint32_t pattern_addr = sprite_pattern_address(row_number, i, entry);

		read_pattern_line(pattern_row_number, pattern_addr,
			entry.horizontal_flip, entry.vertical_flip, on_pixel_read);
	}

	assert((std::size_t)std::distance(dest.begin(), dest_it) <= dest.size());
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
