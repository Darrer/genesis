#ifndef __VDP_IMPL_RENDER_H__
#define __VDP_IMPL_RENDER_H__

#include <cstdint>
#include <array>
#include <span>

#include "vdp/register_set.h"
#include "vdp/settings.h"
#include "vdp/memory.h"
#include "vdp/output_color.h"
#include "color.h"

#include "name_table.h"
#include "sprite_table.h"

namespace genesis::vdp::impl
{

class render
{
public:
	render(genesis::vdp::register_set& regs, genesis::vdp::settings& sett,
	 genesis::vdp::vram_t& vram,  genesis::vdp::vsram_t& vsram, genesis::vdp::cram_t& cram);


	std::uint16_t background_color() const;

	unsigned plane_width_in_pixels(plane_type) const;
	// TODO: rename to height
	unsigned plane_hight_in_pixels(plane_type) const;

	std::span<genesis::vdp::output_color> get_plane_row(impl::plane_type plane_type,
		unsigned row_number, std::span<genesis::vdp::output_color> buffer) const;

	/* Sprite */

	unsigned sprite_width_in_pixels() const { return 512; }
	unsigned sprite_height_in_pixels() const { return 512; }

	std::span<genesis::vdp::output_color> get_sprite_row(unsigned row_number,
		std::span<genesis::vdp::output_color> buffer) const;

private:
	std::array<std::uint8_t, 4> read_pattern_row(unsigned row_number, std::uint32_t pattern_addres, bool hflip, bool vflip) const;
	vdp::output_color read_color(unsigned palette_idx, unsigned color_idx) const;

	bool should_draw_sprite(unsigned row_number, const sprite_table_entry& entry) const;
	void draw_sprite(unsigned row_number, const sprite_table_entry& entry,
		std::span<genesis::vdp::output_color> buffer) const;

	std::uint32_t sprite_pattern_address(unsigned row_number, unsigned pattern_number,
		const sprite_table_entry& entry) const;

private:
	genesis::vdp::register_set& regs;
	genesis::vdp::settings& sett;
	genesis::vdp::vram_t& vram;
	genesis::vdp::vsram_t& vsram;
	genesis::vdp::cram_t& cram;

	mutable std::array<genesis::vdp::output_color, 1024> plane_buffer;
};

}

#endif // __VDP_IMPL_RENDER_H__
