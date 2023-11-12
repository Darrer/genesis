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

private:
	std::uint32_t read_tail_row(unsigned row_number, name_table_entry entry) const;
	vdp::output_color read_color(unsigned palette_idx, unsigned color_idx) const;

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
