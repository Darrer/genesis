#ifndef __TEST_RENDERER_BUILDER_H__
#define __TEST_RENDERER_BUILDER_H__

#include "helpers/random.h"
#include "test_vdp.h"

#include <vector>

using genesis::vdp::impl::plane_type;

namespace genesis::test
{

class renderer_builder
{
public:
	renderer_builder(vdp& vdp) : m_vdp(vdp)
	{
		reset();
	}

	// clear everything so after that renderer starts rendering output_color(0)
	void reset()
	{
		// clear all rams
		m_vdp.zero_vram();
		m_vdp.zero_cram();
		m_vdp.zero_vsram();

		// set background color to 0/0
		auto& regs = m_vdp.registers();
		regs.R7.PAL = 0;
		regs.R7.COL = 0;

		// TODO: reset all plane/sprite addresses to 0

		// setup table addresses
		set_plane_address(plane_type::a, get_plane_address(plane_type::a));
		set_plane_address(plane_type::b, get_plane_address(plane_type::b));
		set_plane_address(plane_type::w, get_plane_address(plane_type::w));
		set_hscroll_address(get_hscroll_address());
		disable_window();
	}

	// setup specified plane to render provided tail
	template <class T>
	void setup_plane(plane_type plane, const T& tail, bool hflip = false, bool vflip = false, std::uint8_t palette = 0,
					 bool priority = false)
	{
		std::uint32_t tail_addr = get_tail_address(plane);
		copy_tail(tail_addr, tail);

		std::uint16_t plane_entry = get_plane_entry(tail_addr, hflip, vflip, palette, priority);
		std::uint32_t plane_addr = get_plane_address(plane);

		set_plane_address(plane, plane_addr);
		set_plane_dimension(plane);
		fill_plane_table(plane, plane_entry);
	}

	void set_window_position()
	{
	}

	void disable_window()
	{
		auto& regs = m_vdp.registers();
		regs.R17.R = 0;
		regs.R17.HP = 0;
		regs.R18.D = 0;
		regs.R18.VP = 0;
	}

	std::uint32_t set_plane_address(plane_type plane, std::uint32_t address)
	{
		auto& regs = m_vdp.registers();
		auto& sett = m_vdp.sett();

		switch(plane)
		{
		case plane_type::a:
			regs.R2.PA5_3 = address >> 13;
			return sett.plane_a_address();
		case plane_type::b:
			regs.R4.PB2_0 = address >> 13;
			return sett.plane_b_address();
		case plane_type::w:
			regs.R3.W5_1 = address >> 11;
			return sett.plane_w_address();
		default:
			throw genesis::internal_error();
		}
	}

	void set_plane_dimension(plane_type plane)
	{
		auto& regs = m_vdp.registers();

		switch(plane)
		{
		case plane_type::a:
		case plane_type::b:
			// use constant plane size for now
			regs.R16.H = 0b01; // 64
			regs.R16.W = 0b01; // 64
			break;
		case plane_type::w:
			regs.R12.RS0 = random::in_range<std::uint8_t>(0, 1);
			break;

		default:
			throw genesis::internal_error();
		}
	}

	void set_hscroll_address(std::uint32_t address)
	{
		m_vdp.registers().R13.HS5_0 = address >> 10;
	}

	// set full scren horizontal scroll
	void set_screen_hscroll(std::uint16_t scroll_a, std::uint16_t scroll_b)
	{
		m_vdp.registers().R11.HS = 0b00;

		std::uint32_t addr = m_vdp.sett().horizontal_scroll_address();

		auto& mem = m_vdp.vram();
		mem.write<std::uint16_t>(addr, scroll_a);
		mem.write<std::uint16_t>(addr + 2, scroll_b);
	}

	// set horizontal scroll per row
	void set_line_hscroll(const std::vector<std::uint16_t>& scroll_a, const std::vector<std::uint16_t>& scroll_b)
	{
		if(scroll_a.size() != scroll_b.size())
			throw std::invalid_argument("scroll array must have the same size");
		if(scroll_a.size() < m_vdp.sett().display_height_in_pixels())
			throw std::invalid_argument("scroll arrays must have entries for each display line");

		m_vdp.registers().R11.HS = 0b11;

		std::uint32_t addr = m_vdp.sett().horizontal_scroll_address();
		auto& mem = m_vdp.vram();

		for(int i = 0; i < m_vdp.sett().display_height_in_pixels(); ++i)
		{
			mem.write<std::uint16_t>(addr, scroll_a[i]);
			mem.write<std::uint16_t>(addr + 2, scroll_b[i]);

			addr += 4;
		}
	}

private:
	template <class T>
	void copy_tail(std::uint32_t address, const T& tail)
	{
		if(tail.size() != 64)
			throw genesis::internal_error();

		auto& vram = m_vdp.vram();
		int i = 0;
		std::uint8_t value = 0;
		for(auto color : tail)
		{
			if(color < 0 || color > 0xF)
				throw genesis::internal_error();

			if(i % 2 == 0)
			{
				value = color << 4;
			}
			else
			{
				value |= color;
				vram.write<std::uint8_t>(address, value);
				address += 1;
			}
			++i;
		}
	}

	void fill_plane_table(plane_type plane, std::uint16_t plane_entry)
	{
		auto& sett = m_vdp.sett();
		auto& vram = m_vdp.vram();
		auto& render = m_vdp.render();

		std::uint32_t plane_address = 0;
		switch(plane)
		{
		case plane_type::a:
			plane_address = sett.plane_a_address();
			break;
		case plane_type::b:
			plane_address = sett.plane_b_address();
			break;
		case plane_type::w:
			plane_address = sett.plane_w_address();
			break;
		default:
			throw genesis::internal_error();
		}

		auto plane_entries = (render.plane_height_in_pixels(plane) / 8) * (render.plane_width_in_pixels(plane) / 8);
		for(unsigned i = 0; i < plane_entries; ++i)
		{
			vram.write<std::uint16_t>(plane_address, plane_entry);
			plane_address += sizeof(plane_entry);
		}
	}

	std::uint16_t get_plane_entry(std::uint32_t tail_address, bool horizintal_flip = false, bool vertical_flip = false,
								  std::uint8_t palette = 0, bool priority = false)
	{
		if(palette > 4)
			throw std::invalid_argument("palette must be in range [0; 4]");

		std::uint16_t plane_entry = 0;
		plane_entry |= tail_address >> 5;
		plane_entry |= palette << 13;

		if(horizintal_flip)
			plane_entry |= 1 << 11;

		if(vertical_flip)
			plane_entry |= 1 << 12;

		if(priority)
			plane_entry |= 1 << 15;

		return plane_entry;
	}

private:
	std::uint32_t get_tail_address(plane_type plane)
	{
		const std::uint32_t TAIL_SIZE = 32;

		// we can allocate 3 tails 32 bytes each till overflow
		std::uint32_t base_addr = 0b1111111110100000;
		switch(plane)
		{
		case plane_type::a:
			return base_addr;
		case plane_type::b:
			return base_addr + TAIL_SIZE;
		case plane_type::w:
			return base_addr + (TAIL_SIZE * 2);
		default:
			throw genesis::internal_error();
		}
	}

	std::uint32_t get_plane_address(plane_type plane)
	{
		// max plane size is 4096 tails or 8192 bytes
		const std::uint32_t MAX_PLANE_SIZE = 8192;

		std::uint32_t base_addr = 0x0;
		switch(plane)
		{
		case plane_type::a:
			return base_addr;
		case plane_type::b:
			return base_addr + MAX_PLANE_SIZE;
		case plane_type::w:
			return base_addr + (MAX_PLANE_SIZE * 2);
		default:
			throw genesis::internal_error();
		}
	}

	// display can be up to 240 pixels height, each line in table is 4 bytes
	// const std::uint32_t HSCROLL_TABLE_SIZE = 960;
	std::uint32_t get_hscroll_address()
	{
		return 24576;
	}

private:
	vdp& m_vdp;
};

} // namespace genesis::test


#endif // __TEST_RENDERER_BUILDER_H__
