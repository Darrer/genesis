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

std::vector<std::uint8_t> random_tail()
{
	return random::next_few_in_range<std::uint8_t>(64, 0, 0xF);
}

std::uint8_t random_palette()
{
	return random::in_range<std::uint8_t>(0, 3);
}

std::uint8_t get_tail_index(std::uint8_t row, std::uint8_t col,
	bool horizontal_flip = false, bool vertical_flip = false)
{
	if(row > 8 || col > 8)
		throw genesis::internal_error();

	if(vertical_flip)
		row = 7 - row;

	if(horizontal_flip)
		col = 7 - col;

	return (row * 8) + col;
}

genesis::vdp::output_color read_color(vdp& vdp, std::uint8_t palette_idx, std::uint8_t col_idx)
{
	if(col_idx == 0)
		return genesis::vdp::TRANSPARENT_COLOR;
	return vdp.cram().read_color(palette_idx, col_idx);
}

template<class T>
void copy_tail(vdp& vdp, std::uint32_t address, const T& tail)
{
	if(tail.size() != 64) // support only 8x8 pixels for now
		throw genesis::internal_error();

	auto& vram = vdp.vram();
	int i = 0;
	std::uint8_t value = 0;
	for(auto color : tail)
	{
		if(color < 0 || color > 0xF)
			throw genesis::internal_error();

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

std::uint16_t get_plane_entry(std::uint32_t tail_address,
	bool horizintal_flip = false, bool vertical_flip = false, std::uint8_t palette = 0)
{
	if(palette > 4)
		throw genesis::internal_error();

	std::uint16_t plane_entry = 0;
	plane_entry |= tail_address >> 5;
	plane_entry |= palette << 13;

	if(horizintal_flip)
		plane_entry |= 1 << 11;

	if(vertical_flip)
		plane_entry |= 1 << 12;

	return plane_entry;
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
		cram.write(addr, random::next<std::uint16_t>());
	}
}

template<class T>
void setup_plane_test(vdp& vdp, bool hflip, bool vflip, 
	std::uint8_t palette, const T& tail)
{
	auto& cram = vdp.cram();
	auto& regs = vdp.registers();
	auto& sett = vdp.sett();
	auto& render = vdp.render();

	// use constant address for simple test
	const std::uint32_t tail_address = 0b1111111111100000;
	copy_tail(vdp, tail_address, tail);

	// use constant plane address for simple test
	regs.R4.PB2_0 = 0b100;
	auto plane_address = sett.plane_b_address(); // TODO: only plane B for now

	// use constant plane size for now
	regs.R16.H = 0b01; // 64
	regs.R16.W = 0b01; // 64

	std::uint16_t plane_entry = get_plane_entry(tail_address, hflip, vflip, palette);
	fill_plane(vdp, sett.plane_b_address(), plane_entry);
	fill_cram(vdp);
}

template<class T, class Callable>
void setup_and_run_plane_test(vdp& vdp, bool hflip, bool vflip,
	const T& tail, Callable expected_color)
{
	// Setup
	const std::uint8_t palette = random_palette();
	setup_plane_test(vdp, hflip, vflip, palette, tail);

	auto& sett = vdp.sett();
	auto& render = vdp.render();

	// Run test
	for(int row_idx = 0; row_idx < sett.plane_height_in_tiles() * 8; ++row_idx)
	{
		auto row = render.get_plane_b_row(row_idx);

		int col_idx = 0;
		for(auto actual_color : row)
		{
			genesis::vdp::output_color color = expected_color(row_idx, col_idx++, palette);
					
			ASSERT_EQ(actual_color, color)
				<< "row: " << row_idx << ", col: " << col_idx - 1;
		}
	}
}

TEST(VDP_RENDER, PLANE_DRAW_SAME_TAIL)
{
	vdp vdp;
	auto& cram = vdp.cram();
	
	std::array<std::uint8_t, 64> tail = {
		0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,
		0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x0,
		0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,
		0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x1,
		0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9,
		0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x0, 0x1,
		0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9,
		0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x0, 0x1
	};

	setup_and_run_plane_test(vdp, false, false, tail, [&](int row, int col, std::uint8_t palette)
	{
		auto tail_idx = get_tail_index(row % 8, col % 8);
		return read_color(vdp, palette, tail.at(tail_idx));
	});
}

TEST(VDP_RENDER, PLANE_HORIZONTAL_FLIP_TAIL)
{
	vdp vdp;
	auto& cram = vdp.cram();
	
	auto tail = random_tail();

	setup_and_run_plane_test(vdp, true, false, tail, [&](int row, int col, std::uint8_t palette)
	{
		auto tail_idx = get_tail_index(row % 8, col % 8, true);
		return read_color(vdp, palette, tail.at(tail_idx));
	});
}

TEST(VDP_RENDER, PLANE_VERTICAL_FLIP_TAIL)
{
	vdp vdp;
	auto& cram = vdp.cram();
	
	auto tail = random_tail();

	setup_and_run_plane_test(vdp, false, true, tail, [&](int row, int col, std::uint8_t palette)
	{
		auto tail_idx = get_tail_index(row % 8, col % 8, false, true);
		return read_color(vdp, palette, tail.at(tail_idx));
	});
}

TEST(VDP_RENDER, PLANE_VERTICAL_AND_HORIZONTAL_FLIP_TAIL)
{
	vdp vdp;
	auto& cram = vdp.cram();

	const std::array<std::uint8_t, 64> tail = {
		0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
		0xb, 0xc, 0xd, 0xe, 0xf, 0x0, 0x1, 0x8,
		0xa, 0xf, 0x0, 0x1, 0x2, 0x3, 0x2, 0x9,
		0x9, 0xe, 0xb, 0xc, 0xd, 0x4, 0x3, 0xa,
		0x8, 0xd, 0xa, 0xf, 0xe, 0x5, 0x4, 0xb,
		0x7, 0xc, 0x9, 0x8, 0x7, 0x6, 0x5, 0xc,
		0x6, 0xb, 0xa, 0x9, 0x8, 0x7, 0x6, 0xd,
		0x5, 0x4, 0x3, 0x2, 0x1, 0x0, 0xf, 0xe
	};

	const std::array<std::uint8_t, 64> fliped_tail = {
		0xe, 0xf, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5,
		0xd, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0x6,
		0xc, 0x5, 0x6, 0x7, 0x8, 0x9, 0xc, 0x7,
		0xb, 0x4, 0x5, 0xe, 0xf, 0xa, 0xd, 0x8,
		0xa, 0x3, 0x4, 0xd, 0xc, 0xb, 0xe, 0x9,
		0x9, 0x2, 0x3, 0x2, 0x1, 0x0, 0xf, 0xa,
		0x8, 0x1, 0x0, 0xf, 0xe, 0xd, 0xc, 0xb,
		0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0
	};

	setup_and_run_plane_test(vdp, true, true, tail, [&](int row, int col, std::uint8_t palette)
	{
		auto tail_idx = get_tail_index(row % 8, col % 8);
		return read_color(vdp, palette, fliped_tail.at(tail_idx));
	});
}
