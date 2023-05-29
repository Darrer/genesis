#ifndef __VDP_SETTINGS_H__
#define __VDP_SETTINGS_H__

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

enum class interlace_mode
{
	disabled,
	normal,
	doubled,
};

enum class dma_mode
{
	mem_to_vram, // M68K memory -> VRAM
	vram_fill,
	vram_copy,
};

enum class display_mode
{
	mode4, // Master System
	mode5 // Mega Drive 
};


class settings
{
public:
	settings(register_set& regs) : regs(regs) { }

	/* VRAM Tables Addresses */

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

	std::uint32_t window_address() const
	{
		std::uint32_t addr = regs.R3.W5_1;
		if(display_width() == display_width::c40)
			addr = addr & ~1;
		addr = addr << 11;
		// TODO: use WA6?
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

	// Height for planes A & B
	plane_dimension plane_height() const
	{
		switch (regs.R16.H)
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
		switch (regs.R16.W)
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

	std::uint32_t dma_source() const
	{
		std::uint32_t source = std::uint32_t(regs.R23.H) << 17;
		source |= std::uint32_t(regs.R22.M) << 9;
		source |= std::uint32_t(regs.R21.L) << 1;

		if(dma_mode() == dma_mode::mem_to_vram)
		{
			// T0 acts as H6
			source |= std::uint32_t(regs.R23.T0) << 23;
		}

		return source;
	}

	dma_mode dma_mode() const
	{
		if(regs.R23.T1 == 0)
			return dma_mode::mem_to_vram;

		if(regs.R23.T0 == 0)
			return dma_mode::vram_fill;

		return dma_mode::vram_copy;
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

	/* Display settings */

	display_mode display_mode() const
	{
		if(regs.R1.M5 == 1)
			return display_mode::mode5;
		return display_mode::mode4;
	}

	display_height display_height() const
	{
		if(regs.R1.M2 == 1)
			return display_height::c30;
		return display_height::c28;
	}

	display_width display_width() const
	{
		// TODO: take into accout RS1?
		if(regs.R12.RS0 == 1)
			return display_width::c40;
		return display_width::c32;
	}

	interlace_mode interlace_mode() const
	{
		switch (regs.R12.LS)
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
	vertical_scrolling vertical_scrolling() const
	{
		if(regs.R11.VS == 1)
			return vertical_scrolling::two_cell;
		return vertical_scrolling::full_screen;
	}

	horizontal_scrolling horizontal_scrolling() const
	{
		switch (regs.R11.HS)
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

private:
	register_set& regs;
};

}

#endif // __VDP_SETTINGS_H__
