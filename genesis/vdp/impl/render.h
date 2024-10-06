#ifndef __VDP_IMPL_RENDER_H__
#define __VDP_IMPL_RENDER_H__

#include <cstdint>
#include <cassert>
#include <array>
#include <span>

#include "vdp/register_set.h"
#include "vdp/settings.h"
#include "vdp/memory.h"
#include "vdp/output_color.h"

#include "name_table.h"
#include "sprite_table.h"

namespace genesis::vdp::impl
{

// TODO: rename to renderer
class render
{
public:
	render(genesis::vdp::register_set& regs, genesis::vdp::settings& sett,
	 genesis::vdp::vram_t& vram,  genesis::vdp::vsram_t& vsram, genesis::vdp::cram_t& cram);


	output_color background_color() const;

	/* A/B/W Planes */

	unsigned plane_width_in_pixels(plane_type) const;
	unsigned plane_height_in_pixels(plane_type) const;

	std::span<genesis::vdp::output_color> get_plane_row(impl::plane_type plane_type,
		unsigned row_number, std::span<genesis::vdp::output_color> buffer) const;

	/* Sprites */

	unsigned sprite_width_in_pixels() const { return 512; }
	unsigned sprite_height_in_pixels() const { return 512; }

	std::span<genesis::vdp::output_color> get_sprite_row(unsigned row_number,
		std::span<genesis::vdp::output_color> buffer) const;

	/* Active Display */

	unsigned active_display_width() const;
	unsigned active_display_height() const;

	std::span<genesis::vdp::output_color> get_active_display_row(unsigned row_number,
		std::span<genesis::vdp::output_color> buffer);

	// should be called when VDP starts rendering new frame
	void reset_limits();

private:
	// the actual pixel produced by vdp does not have priority or transparency,
	// but these properties are required to build the vdp frame,
	// so use different pixel representation for internal purposes
	struct internal_pixel
	{
		internal_pixel()
		{
			palette_id = color_id = priority = 0;
		}

		internal_pixel(int p, int c, bool pr)
		{
			palette_id = (std::uint8_t)p;
			color_id = (std::uint8_t)c;
			priority = pr;
		}

		std::uint8_t palette_id : 2;
		std::uint8_t color_id : 4;

		// only tails have priority, but it would be easier to assign each pixel a priority
		bool priority : 1;

		// pixel is transparent if color_id is 0
		bool transparent() const { return color_id == 0; }
	};

	static_assert(sizeof(internal_pixel) == 1);

	// line_number - zero based
	template<class Callable>
	void read_pattern_line(unsigned line_number, std::uint32_t pattern_addres,
		bool hflip, bool vflip, Callable on_pixel_read) const
	{
		assert(line_number < 8);

		if(vflip)
			line_number = 7 - line_number;

		pattern_addres += line_number * 0x4 /* single line occupies 4 bytes */;
		std::uint32_t raw_line = vram.read_raw<std::uint32_t>(pattern_addres);

		if(hflip)
		{
			constexpr std::array<int, 8> offsets = {24, 28, 16, 20, 8, 12, 0, 4};
			for(int offset : offsets)
				on_pixel_read((raw_line >> offset) & 0xF);
		}
		else
		{
			constexpr std::array<int, 8> offsets = {4, 0, 12, 8, 20, 16, 28, 24};
			for(int offset : offsets)
				on_pixel_read((raw_line >> offset) & 0xF);
		}
	}

	vdp::output_color read_color(unsigned palette_idx, unsigned color_idx) const;

	/* Sprites helpers */
	std::span<internal_pixel> get_active_sprites_row(unsigned row_number, std::span<internal_pixel> buffer);
	bool should_render_sprite(unsigned row_number, unsigned vertical_position, unsigned vertical_size) const;

	// For performance reason it better to duplicate these functions
	void read_sprite(unsigned row_number, const sprite_table_entry& entry, std::span<vdp::output_color> buffer) const;

	// Returns true if sprites collision occurs
	bool read_sprite(unsigned row_number,const sprite_table_entry& entry,
		std::span<internal_pixel> dest, unsigned pixels_limit) const;

	std::uint32_t sprite_pattern_address(unsigned row_number, unsigned pattern_number,
		const sprite_table_entry& entry) const;

	/* Plane helpers */

	std::span<internal_pixel> get_active_plane_row(plane_type plane_type, unsigned row_number,
		std::span<internal_pixel> buffer) const;

	void render_active_window_row(unsigned line_number, std::span<internal_pixel> plane_a_buffer) const;

	genesis::vdp::output_color resolve_priority(genesis::vdp::output_color background_color,
		internal_pixel plane_a, internal_pixel plane_b, internal_pixel sprite) const;

	/* internal buffers used during rendering */

	// 512 is sprite plane width
	mutable std::array<internal_pixel, 512> sprite_buffer;

	// 1024 is max palne width
	mutable std::array<internal_pixel, 1024> pixel_a_buffer;
	mutable std::array<internal_pixel, 1024> pixel_b_buffer;

private:
	/* Helper functions */
	template<class T>
	static void check_buffer_size(std::span<T> buffer, std::size_t minimum_size)
	{
		if(buffer.size() < minimum_size)
			throw std::invalid_argument("provided buffer does not have enough space");
	}

private:
	genesis::vdp::register_set& regs;
	genesis::vdp::settings& sett;
	genesis::vdp::vram_t& vram;
	genesis::vdp::vsram_t& vsram;
	genesis::vdp::cram_t& cram;
};

}

#endif // __VDP_IMPL_RENDER_H__
