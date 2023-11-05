#include <gtest/gtest.h>

#include "exception.hpp"

#include "test_vdp.h"
#include "helpers/random.h"

using namespace genesis::test;

void setup_background_color(vdp& vdp, std::uint8_t palette, std::uint8_t color_number)
{
	auto& regs = vdp.registers();
	regs.R7.PAL = palette;
	regs.R7.COL = color_number;
}

TEST(VDP_RENDER, BACKGROUND_COLOR)
{
	vdp vdp;
	auto& render = vdp.render();
	auto& cram = vdp.cram();
	auto& regs = vdp.registers();

	// there are 64 colors in total (4 palette * 16 colors each)
	auto colors = random::next_few<std::uint16_t>(64);
	ASSERT_EQ(64, colors.size());

	for(std::uint16_t color = 0; color < colors.size(); ++color)
		cram.write(color * 2, colors.at(color));

	std::uint16_t color = 0;
	for(std::uint8_t palette = 0; palette < 4; ++palette)
	{
		regs.R7.PAL = palette;
		for(std::uint8_t pos = 0; pos < 16; ++pos)
		{
			regs.R7.COL = pos;

			auto expected_color = colors.at(color++);
			ASSERT_EQ(expected_color, render.background_color())
				<< "palette: " << (int)palette << ", color number: " << (int)pos;
		}
	}
}


TEST(VDP_RENDER, PLANE_B_ROW_SIZE)
{
	vdp vdp;
	auto& render = vdp.render();
	auto& vram = vdp.vram();
	auto& regs = vdp.registers();
	auto& sett = vdp.sett();

	regs.R16.W = 0b00; // 32 tiles width
	auto row = render.get_plane_b_row(0);
	ASSERT_EQ(32 * 8, row.size());

	regs.R16.W = 0b01; // 128 tiles width
	row = render.get_plane_b_row(0);
	ASSERT_EQ(64 * 8, row.size());

	regs.R16.W = 0b11; // 128 tiles width
	row = render.get_plane_b_row(0);
	ASSERT_EQ(128 * 8, row.size());
}

void copy_tail(vdp& vdp, std::uint32_t address, std::initializer_list<std::uint8_t> tail)
{
	if(tail.size() != 64) // support only 8x8 pixels for now
		throw genesis::internal_error();

	auto& vram = vdp.vram();
	int i = 0;
	std::uint8_t value = 0;
	for(auto color : tail)
	{
		if(i % 2 == 0)
		{
			value = color;
		}
		else
		{
			value |= color << 4;
			vram.write<std::uint8_t>(address, value);
			address += 1;
		}
		++i;
	}
}

void fill_plane(vdp& vdp, std::uint32_t plane_address, std::uint16_t plane_entry)
{
	auto& sett = vdp.sett();
	auto& vram = vdp.vram();

	if(plane_address != sett.plane_a_address() && plane_address != sett.plane_b_address())
		throw genesis::internal_error();

	auto plane_entries = sett.plane_height_in_tiles() * sett.plane_width_in_tiles();
	for(int i = 0; i < plane_entries; ++i)
	{
		vram.write<std::uint16_t>(plane_address, plane_entry);
		plane_address += sizeof(plane_entry);
	}
}

void fill_cram(vdp& vdp)
{
	auto& cram = vdp.cram();

	for(int i = 0; i < 64; ++i)
	{
		std::uint16_t addr = i * 2;
		std::uint16_t color = genesis::vdp::color(random::next<std::uint16_t>()).value();

		cram.write(addr, color);
	}
}

TEST(VDP_RENDER, PLANE_B_DRAW_SAME_TAIL)
{
	vdp vdp;
	auto& vram = vdp.vram();
	auto& cram = vdp.cram();
	auto& regs = vdp.registers();
	auto& sett = vdp.sett();
	auto& render = vdp.render();

	const std::uint32_t tail_address = 0b1111111111100000;
	std::initializer_list<std::uint8_t> tail = {
		0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
		0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2,
		0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
		0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4,
		0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5,
		0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6,
		0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
		0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8
	};

	copy_tail(vdp, tail_address, tail);

	std::uint16_t plane_entry = 0;
	plane_entry |= tail_address >> 5;

	regs.R4.PB2_0 = 0b100;
	auto plane_address = sett.plane_b_address();

	// TODO: not sure it's valid plane size
	regs.R16.H = 0b01; // 64
	regs.R16.W = 0b01; // 64

	fill_plane(vdp, sett.plane_b_address(), plane_entry);
	fill_cram(vdp);

	for(int row_idx = 0; row_idx < sett.plane_height_in_tiles() * 8; ++row_idx)
	{
		auto row = render.get_plane_b_row(row_idx);

		auto first_color = row.front();
		for(auto color : row)
			ASSERT_EQ(first_color.value(), color.value());

		auto expected_color_idx = (row_idx % 8) + 1;
		auto expected_color = cram.read_color(0, expected_color_idx);


		for(auto actual_color : row)
			ASSERT_EQ(expected_color, actual_color.value())
				<< "row: " << row_idx << ", expected color idx: " << expected_color_idx
				<< ", expected color: " << expected_color;
	}
}
