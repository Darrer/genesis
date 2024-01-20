#ifndef __VDP_IMPL_RENDER_H__
#define __VDP_IMPL_RENDER_H__

#include <cstdint>
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

class render
{
public:
	render(genesis::vdp::register_set& regs, genesis::vdp::settings& sett,
	 genesis::vdp::vram_t& vram,  genesis::vdp::vsram_t& vsram, genesis::vdp::cram_t& cram);


	output_color background_color() const;

	/* A/B/W Planes */

	unsigned plane_width_in_pixels(plane_type) const;
	// TODO: rename to height
	unsigned plane_hight_in_pixels(plane_type) const;

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
	struct pixel
	{
		vdp::output_color color;

		// we greatly simplify algorithm by using priority flag per pixel
		// even though plane A/B have priority flag per tail
		bool priority_flag;
	};

	const pixel TRANSPARENT_PIXEL = { TRANSPARENT_COLOR, false };

	std::array<vdp::output_color, 8> read_pattern_row(unsigned pattern_row_number, std::uint32_t pattern_addres,
		bool hflip, bool vflip, std::uint8_t palette_id) const;

	// std::span<pixel>::iterator write_pattern()

	vdp::output_color read_color(unsigned palette_idx, unsigned color_idx) const;

	/* Sprites helpers */
	std::span<pixel> get_active_sprites_row(unsigned row_number, std::span<pixel> buffer);
	bool should_read_sprite(unsigned row_number, unsigned vertical_position, unsigned vertical_size) const;

	// For performance reason it better to duplicate these functions
	void read_sprite(unsigned row_number, const sprite_table_entry& entry, std::span<vdp::output_color> buffer) const;

	// Returns true if sprites collision occurs
	bool read_sprite(unsigned row_number,const sprite_table_entry& entry,
		std::span<pixel> dest, unsigned pixels_limit) const;

	std::uint32_t sprite_pattern_address(unsigned row_number, unsigned pattern_number,
		const sprite_table_entry& entry) const;

	/* Plane helpers */

	std::span<pixel>::iterator get_active_plane_row(plane_type plane_type, unsigned row_number,
		std::span<pixel> buffer) const;
	std::span<pixel>::iterator get_scrolled_plane_row(impl::plane_type plane_type, unsigned row_number,
		std::span<pixel> buffer) const;

	std::span<pixel> get_active_window_row(unsigned line_number, std::span<pixel> plane_a_buffer) const;

	genesis::vdp::output_color resolve_priority(genesis::vdp::output_color background_color,
		pixel plane_a, pixel plane_b, pixel sprite) const;

	// internal buffers used during rendering
	mutable std::array<pixel, 512> sprite_buffer;
	mutable std::array<pixel, 1024> pixel_a_buffer;
	mutable std::array<pixel, 1024> pixel_b_buffer;

private:
	/* Helper functions */
	template<class T>
	static void check_buffer_size(std::span<T> buffer, std::size_t minimum_size)
	{
		if(buffer.size() < minimum_size)
			throw genesis::internal_error();
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
