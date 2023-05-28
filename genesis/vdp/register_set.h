#ifndef __VDP_REGISTER_SET_H__
#define __VDP_REGISTER_SET_H__

#include "registers.h"
#include "exception.hpp"


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

class register_set
{
public:
	register_set()
	{
		for(std::uint8_t i = 0; i <= 23; ++i)
			set_register(i, 0);
		sr_raw = 0;
	}

	void set_register(std::uint8_t reg, std::uint8_t value)
	{
		get_register(reg) = value;
	}

	std::uint8_t& get_register(std::uint8_t reg)
	{
		switch (reg)
		{
		case 0:
			return *reinterpret_cast<std::uint8_t*>(&R0);
		case 1:
			return *reinterpret_cast<std::uint8_t*>(&R1);
		case 2:
			return *reinterpret_cast<std::uint8_t*>(&R2);
		case 3:
			return *reinterpret_cast<std::uint8_t*>(&R3);
		case 4:
			return *reinterpret_cast<std::uint8_t*>(&R4);
		case 5:
			return *reinterpret_cast<std::uint8_t*>(&R5);
		case 6:
			return *reinterpret_cast<std::uint8_t*>(&R6);
		case 7:
			return *reinterpret_cast<std::uint8_t*>(&R7);
		case 8:
			return *reinterpret_cast<std::uint8_t*>(&R8);
		case 9:
			return *reinterpret_cast<std::uint8_t*>(&R9);
		case 10:
			return *reinterpret_cast<std::uint8_t*>(&R10);
		case 11:
			return *reinterpret_cast<std::uint8_t*>(&R11);
		case 12:
			return *reinterpret_cast<std::uint8_t*>(&R12);
		case 13:
			return *reinterpret_cast<std::uint8_t*>(&R13);
		case 14:
			return *reinterpret_cast<std::uint8_t*>(&R14);
		case 15:
			return *reinterpret_cast<std::uint8_t*>(&R15);
		case 16:
			return *reinterpret_cast<std::uint8_t*>(&R16);
		case 17:
			return *reinterpret_cast<std::uint8_t*>(&R17);
		case 18:
			return *reinterpret_cast<std::uint8_t*>(&R18);
		case 19:
			return *reinterpret_cast<std::uint8_t*>(&R19);
		case 20:
			return *reinterpret_cast<std::uint8_t*>(&R20);
		case 21:
			return *reinterpret_cast<std::uint8_t*>(&R21);
		case 22:
			return *reinterpret_cast<std::uint8_t*>(&R22);
		case 23:
			return *reinterpret_cast<std::uint8_t*>(&R23);

		default: throw internal_error();
		}
	}

	/* Helper methods */

	// TODO: move helper methods to different class?
	// vdp_settings?

	std::uint32_t plane_a_address() const
	{
		std::uint32_t addr = R2.PA5_3;
		addr = addr << 13;
		// TODO: use PA6?
		return addr;
	}

	std::uint32_t plane_b_address() const
	{
		std::uint32_t addr = R4.PB2_0;
		addr = addr << 13;
		// TODO: use PB3?
		return addr;
	}

	std::uint32_t window_address() const
	{
		std::uint32_t addr = R3.W5_1;
		if(display_width() == display_width::c40)
			addr = addr & ~1;
		addr = addr << 11;
		// TODO: use WA6?
		return addr;
	}

	std::uint32_t sprite_address() const
	{
		std::uint32_t addr = R5.ST6_0;
		if(display_width() == display_width::c40)
			addr = addr & ~1;
		addr = addr << 9;
		// TODO: use ST7?
		return addr;
	}

	std::uint32_t horizontal_scroll_address() const
	{
		std::uint32_t addr = R13.HS5_0;
		addr = addr << 10;
		// TODO: use HS6
		return addr;
	}

	// Height for planes A & B
	plane_dimension plane_height() const
	{
		switch (R16.H)
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
		switch (R16.W)
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
		return R1.M1 == 1;
	}

	std::uint16_t dma_length() const
	{
		std::uint16_t length = std::uint16_t(R20.H) << 8;
		length |= R19.L;
		return length;
	}

	std::uint32_t dma_source() const
	{
		std::uint32_t source = std::uint32_t(R23.H) << 17;
		source |= std::uint32_t(R22.M) << 9;
		source |= std::uint32_t(R21.L) << 1;

		if(dma_mode() == dma_mode::mem_to_vram)
		{
			// T0 acts as H6
			source |= std::uint32_t(R23.T0) << 23;
		}

		return source;
	}

	dma_mode dma_mode() const
	{
		if(R23.T1 == 0)
			return dma_mode::mem_to_vram;

		if(R23.T0 == 0)
			return dma_mode::vram_fill;

		return dma_mode::vram_copy;
	}

	/* Interrupts */

	bool vertical_interrupt_enabled() const
	{
		return R1.IE0 == 1;
	}

	bool horizontal_interrupt_enabled() const
	{
		return R0.IE1 == 1;
	}

	bool external_interrupt_enabled() const
	{
		return R11.IE2 == 1;
	}

	/* Display settings */

	display_mode display_mode() const
	{
		if(R1.M5 == 1)
			return display_mode::mode5;
		return display_mode::mode4;
	}

	display_height display_height() const
	{
		if(R1.M2 == 1)
			return display_height::c30;
		return display_height::c28;
	}

	display_width display_width() const
	{
		// TODO: take into accout RS1?
		if(R12.RS0 == 1)
			return display_width::c40;
		return display_width::c32;
	}

	interlace_mode interlace_mode() const
	{
		switch (R12.LS)
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
		if(R11.VS == 1)
			return vertical_scrolling::two_cell;
		return vertical_scrolling::full_screen;
	}

	horizontal_scrolling horizontal_scrolling() const
	{
		switch (R11.HS)
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
		return R0.M3 == 1;
	}

	bool in_128kb_vram_mode() const
	{
		return R1.VR == 1;
	}

	/* Registers */

	R0 R0;
	R1 R1;
	R2 R2;
	R3 R3;
	R4 R4;
	R5 R5;
	R6 R6;
	R7 R7;
	R8 R8;
	R9 R9;
	R10 R10;
	R11 R11;
	R12 R12;
	R13 R13;
	R14 R14;
	R15 R15;
	R16 R16;
	R17 R17;
	R18 R18;
	R19 R19;
	R20 R20;
	R21 R21;
	R22 R22;
	R23 R23;

	union
	{
		status_register SR;
		std::uint16_t sr_raw;
	};
};

}

#endif // __VDP_REGISTER_SET_H__