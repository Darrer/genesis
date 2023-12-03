#ifndef __VDP_SETTINGS_H__
#define __VDP_SETTINGS_H__

#include "endian.hpp"
#include "register_set.h"

namespace genesis::vdp
{

// c - stands for cell
enum class plane_dimension
{
	c32,
	c64,
	c128,
	invalid,
};

enum class display_height
{
	c30, // PAL,  240 pixels
	c28, // NTSC, 224 pixels
};

enum class display_width
{
	c32, // 256 pixels
	c40, // 320 pixels
};

enum class vertical_scrolling
{
	full_screen,
	two_cell,
};

enum class horizontal_scrolling
{
	full_screen,
	line,
	cell,
	invalid,
};

enum class draw_horizontal_direction
{
	left,
	right,
};

enum class draw_vertical_direction
{
	up,
	down,
};

enum class interlace_mode
{
	disabled,
	normal,
	doubled,
};

enum class dma_mode
{
	mem_to_vram, // M68K memory -> VRAM
	vram_fill,	 // TODO: rename to fill
	vram_copy,
};

enum class display_mode
{
	mode4, // Master System
	mode5  // Mega Drive
};


class settings
{
public:
	settings(register_set& regs) : regs(regs)
	{
	}

	/* Tables Addresses */

	std::uint32_t plane_a_address() const
	{
		std::uint32_t addr = regs.R2.PA5_3;
		addr = addr << 13;
		// TODO: use PA6?
		return addr;
	}

	std::uint32_t plane_b_address() const
	{
		std::uint32_t addr = regs.R4.PB2_0;
		addr = addr << 13;
		// TODO: use PB3?
		return addr;
	}

	std::uint32_t plane_w_address() const
	{
		std::uint32_t addr = regs.R3.W5_1;
		if(display_width() == display_width::c40)
			addr = addr & ~1;
		addr = addr << 11;
		// TODO: use W6?
		return addr;
	}

	std::uint32_t sprite_address() const
	{
		std::uint32_t addr = regs.R5.ST6_0;
		if(display_width() == display_width::c40)
			addr = addr & ~1;
		addr = addr << 9;
		// TODO: use ST7?
		return addr;
	}

	std::uint32_t horizontal_scroll_address() const
	{
		std::uint32_t addr = regs.R13.HS5_0;
		addr = addr << 10;
		// TODO: use HS6
		return addr;
	}

	/* Plane dimenstion */

	// Height for planes A & B
	plane_dimension plane_height() const
	{
		switch(regs.R16.H)
		{
		case 0b00:
			return plane_dimension::c32;
		case 0b01:
			return plane_dimension::c64;
		case 0b11:
			return plane_dimension::c128;
		default:
			return plane_dimension::invalid;
		}
	}

	// Width for planes A & B
	plane_dimension plane_width() const
	{
		switch(regs.R16.W)
		{
		case 0b00:
			return plane_dimension::c32;
		case 0b01:
			return plane_dimension::c64;
		case 0b11:
			return plane_dimension::c128;
		default:
			return plane_dimension::invalid;
		}
	}

	std::uint8_t plane_width_in_tiles() const
	{
		switch (plane_width())
		{
		case plane_dimension::c32:
			return 32;
		case plane_dimension::c64:
			return 64;
		case plane_dimension::c128:
			return 128;
		default: throw internal_error();
		}
	}

	std::uint8_t plane_height_in_tiles() const
	{
		switch (plane_height())
		{
		case plane_dimension::c32:
			return 32;
		case plane_dimension::c64:
			return 64;
		case plane_dimension::c128:
			return 128;
		default: throw internal_error();
		}
	}

	/* Window Plane Position & Direction */

	std::uint8_t window_horizontal_pos_in_cells() const
	{
		return regs.R17.HP;
	}

	draw_horizontal_direction window_horizontal_draw_direction() const
	{
		// if(regs.R17.R == 1)
		// TODO: figure out why window plane works only when we invert R17.R bit
		if(regs.R17.R == 0)
			return draw_horizontal_direction::right;
		return draw_horizontal_direction::left;
	}

	std::uint8_t window_vertical_pos_in_cells() const
	{
		return regs.R18.VP;
	}

	draw_vertical_direction window_vertical_draw_direction() const
	{
		if(regs.R18.D == 1)
			return draw_vertical_direction::down;
		return draw_vertical_direction::up;
	}

	/* DMA */

	bool dma_enabled() const
	{
		return regs.R1.M1 == 1;
	}

	std::uint16_t dma_length() const
	{
		std::uint16_t length = std::uint16_t(regs.R20.H) << 8;
		length |= regs.R19.L;
		return length;
	}

	void dma_length(std::uint16_t length)
	{
		regs.R20.H = endian::msb(length);
		regs.R19.L = endian::lsb(length);
	}

	std::uint32_t dma_source() const
	{
		std::uint32_t source = 0;
		source |= std::uint32_t(regs.R21.L);
		source |= std::uint32_t(regs.R22.M) << 8;

		if(regs.R23.T1 == 0)
		{
			// NOTE:
			// - DMA source is not used during DMA FILL
			// - regs.R23 is not used during DMA VRAM COPY (as source address is 16 bits)
			// - So it's used only during DMA M68K -> VRAM
			source |= std::uint32_t(regs.R23.H) << 16;

			// T0 acts as H6
			source |= std::uint32_t(regs.R23.T0) << 22;

			// To get M68K address we have to multiply source by 2
			source = source << 1;
		}

		return source;
	}

	void dma_source(std::uint32_t value)
	{
		if(regs.R23.T1 == 0)
		{
			// restore initial value before writing back to registers
			// 0th bit always should be 0 as M68K -> VRAM DMA always word wide,
			// so do not lose information during the shift
			value = value >> 1;
		}

		regs.R21.L = std::uint8_t(value & 0xFF);
		value = value >> 8;

		regs.R22.M = std::uint8_t(value & 0xFF);
		value = value >> 8;

		if(regs.R23.T1 == 0)
		{
			regs.R23.H = std::uint8_t(value & 0b111111);
			regs.R23.T0 = std::uint8_t((value >> 6) & 1);
		}
	}

	vdp::dma_mode dma_mode() const
	{
		if(regs.R23.T1 == 0)
			return dma_mode::mem_to_vram;

		if(regs.R23.T0 == 0)
			return dma_mode::vram_fill;

		return dma_mode::vram_copy;
	}

	void dma_mode(vdp::dma_mode mode)
	{
		switch(mode)
		{
		case dma_mode::mem_to_vram:
			regs.R23.T1 = 0;
			break;

		case dma_mode::vram_fill:
			regs.R23.T1 = 1;
			regs.R23.T0 = 0;
			break;

		case dma_mode::vram_copy:
			regs.R23.T1 = regs.R23.T0 = 1;
			break;

		default:
			throw internal_error();
		}
	}

	/* Interrupts */

	bool vertical_interrupt_enabled() const
	{
		return regs.R1.IE0 == 1;
	}

	bool horizontal_interrupt_enabled() const
	{
		return regs.R0.IE1 == 1;
	}

	bool external_interrupt_enabled() const
	{
		return regs.R11.IE2 == 1;
	}

	std::uint8_t horizontal_interrupt_counter() const
	{
		return regs.R10.H;
	}

	/* Display settings */

	vdp::display_mode display_mode() const
	{
		if(regs.R1.M5 == 1)
			return display_mode::mode5;
		return display_mode::mode4;
	}

	vdp::display_height display_height() const
	{
		if(regs.R1.M2 == 1)
			return display_height::c30;
		return display_height::c28;
	}

	std::uint16_t display_height_in_pixels() const
	{
		if(display_height() == display_height::c30)
			return 240;
		return 224; // c28
	}

	std::uint8_t display_height_in_tails() const
	{
		if(display_height() == display_height::c30)
			return 30;
		return 28;
	}

	vdp::display_width display_width() const
	{
		// TODO: take into accout RS1?
		if(regs.R12.RS0 == 1)
			return display_width::c40;
		return display_width::c32;
	}

	std::uint16_t display_width_in_pixels() const
	{
		if(display_width() == display_width::c40)
			return 320;
		return 256;
	}

	std::uint8_t display_width_in_tails() const
	{
		if(display_width() == display_width::c40)
			return 40;
		return 32;
	}

	vdp::interlace_mode interlace_mode() const
	{
		switch(regs.R12.LS)
		{
		case 0b01:
			return interlace_mode::normal;
		case 0b11:
			return interlace_mode::doubled;
		default:
			return interlace_mode::disabled;
		}
	}

	/* Scrolling */
	vdp::vertical_scrolling vertical_scrolling() const
	{
		if(regs.R11.VS == 1)
			return vertical_scrolling::two_cell;
		return vertical_scrolling::full_screen;
	}

	vdp::horizontal_scrolling horizontal_scrolling() const
	{
		switch(regs.R11.HS)
		{
		case 0b00:
			return horizontal_scrolling::full_screen;
		case 0b10:
			return horizontal_scrolling::cell;
		case 0b11:
			return horizontal_scrolling::line;

		// TODO: invalid or line?
		default:
			return horizontal_scrolling::invalid;
		}
	}

	bool freeze_hv_counter() const
	{
		return regs.R0.M3 == 1;
	}

	bool in_128kb_vram_mode() const
	{
		return regs.R1.VR == 1;
	}

	std::uint8_t auto_increment_value() const
	{
		return regs.R15.INC;
	}

private:
	register_set& regs;
};

} // namespace genesis::vdp

#endif // __VDP_SETTINGS_H__
