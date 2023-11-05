#ifndef __VDP_IMPL_RENDER_H__
#define __VDP_IMPL_RENDER_H__

#include <cstdint>
#include <array>
#include <span>

#include "vdp/register_set.h"
#include "vdp/settings.h"
#include "vdp/memory.h"
#include "color.h"

namespace genesis::vdp::impl
{

class render
{
public:
	render(genesis::vdp::register_set& regs, genesis::vdp::settings& sett,
	 genesis::vdp::vram_t& vram,  genesis::vdp::vsram_t& vsram, genesis::vdp::cram_t& cram);


	std::uint16_t background_color() const;

	std::span<genesis::vdp::color> get_plane_b_row(std::uint8_t row_number);

private:
	std::uint32_t read_tail_row(std::uint32_t tail_address, std::uint8_t row_number) const;

private:
	genesis::vdp::register_set& regs;
	genesis::vdp::settings& sett;
	genesis::vdp::vram_t& vram;
	genesis::vdp::vsram_t& vsram;
	genesis::vdp::cram_t& cram;

	std::array<genesis::vdp::color, 1024> plane_buffer;
};

}

#endif // __VDP_IMPL_RENDER_H__
