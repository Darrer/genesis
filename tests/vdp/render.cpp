#include <gtest/gtest.h>

#include "test_vdp.h"
#include "renderer_builder.hpp"
#include "helpers/random.h"
#include "exception.hpp"

#include "vdp/impl/plane_type.h"
using genesis::vdp::impl::plane_type;

using namespace genesis::test;

static std::array<genesis::vdp::output_color, 1024> plane_buffer;

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

std::vector<std::uint8_t> transparent_tail()
{
	return {
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
	};
}

std::vector<std::uint8_t> random_tail(std::uint8_t min = 0x0, std::uint8_t max = 0xF)
{
	return random::next_few_in_range<std::uint8_t>(64, min, max);
}

const int TAIL_SIZE = 64; // bytes

std::uint8_t random_palette()
{
	return random::in_range<std::uint8_t>(0, 3);
}

plane_type random_plane()
{
	return random::pick({plane_type::a, plane_type::b, plane_type::w});
}

std::uint8_t get_tail_index(std::uint8_t row, std::uint8_t col,
	bool horizontal_flip = false, bool vertical_flip = false,
	int hscroll = 0)
{
	if(row > 8 || col > 8)
		throw genesis::internal_error();

	hscroll = hscroll % 8;
	col = (col + 8 - hscroll) % 8;

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

genesis::vdp::output_color read_active_color(vdp& vdp, std::uint8_t palette_idx, std::uint8_t col_idx)
{
	if(col_idx == 0)
		return vdp.render().background_color();
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

std::uint16_t get_plane_entry(std::uint32_t tail_address,
	bool horizintal_flip = false, bool vertical_flip = false, std::uint8_t palette = 0,
	bool priority = false)
{
	if(palette > 4)
		throw std::invalid_argument("palette");

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

void fill_cram(vdp& vdp)
{
	auto& cram = vdp.cram();

	for(int i = 0; i < 64; ++i)
	{
		std::uint16_t addr = i * 2;
		cram.write(addr, random::next<std::uint16_t>());
	}
}

auto get_plane_row(vdp& vdp, plane_type plane, unsigned row_number)
{
	return vdp.render().get_plane_row(plane, row_number, plane_buffer);
}

std::uint32_t set_plane_address(vdp& vdp, plane_type plane, std::uint8_t address)
{
	auto& regs = vdp.registers();
	auto& sett = vdp.sett();

	switch (plane)
	{
	case plane_type::a:
		regs.R2.PA5_3 = address; // plane A
		return sett.plane_a_address();
	case plane_type::b:
		regs.R4.PB2_0 = address; // plane B
		return sett.plane_b_address();
	case plane_type::w:
		regs.R3.W5_1 = address;
		return sett.plane_w_address();
	default: throw genesis::internal_error();
	}
}

void set_plane_dimension(vdp& vdp, plane_type plane)
{
	auto& regs = vdp.registers();

	switch (plane)
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

	default: throw genesis::internal_error();
	}
}

template<class T, class Callable>
void setup_and_run_plane_test(vdp& vdp, bool hflip, bool vflip,
	const T& tail, Callable expected_color)
{
	// Setup
	const std::uint8_t palette = random_palette();
	auto plane_type = random_plane();

	renderer_builder builder(vdp);
	builder.setup_plane(plane_type, tail, hflip, vflip, palette);

	fill_cram(vdp);

	// Run test
	auto& render = vdp.render();
	for(unsigned row_idx = 0; row_idx < render.plane_height_in_pixels(plane_type); ++row_idx)
	{
		auto row = get_plane_row(vdp, plane_type, row_idx);

		int col_idx = 0;
		for(auto actual_color : row)
		{
			genesis::vdp::output_color color = expected_color(row_idx, col_idx++, palette);
					
			ASSERT_EQ(actual_color, color)
				<< "row: " << row_idx << ", col: " << col_idx - 1
				<< ", expected " << color.to_internal()
				<< ", actual " << actual_color.to_internal();
		}
	}
}

template<class Callable>
void run_active_display_test(vdp& vdp, Callable expected_color)
{
	auto& render = vdp.render();
	for(unsigned row_idx = 0; row_idx < render.active_display_height(); ++row_idx)
	{
		auto row = render.get_active_display_row(row_idx, plane_buffer);

		int col_idx = 0;
		for(auto actual_color : row)
		{
			genesis::vdp::output_color color = expected_color(row_idx, col_idx++);
					
			ASSERT_EQ(actual_color, color)
				<< "row: " << row_idx << ", col: " << col_idx - 1
				<< ", expected " << color.to_internal()
				<< ", actual " << actual_color.to_internal();
		}
	}
}


TEST(VDP_RENDER, PLANE_AB_ROW_WIDTH)
{
	vdp vdp;
	const auto plane = random::pick({plane_type::a, plane_type::b});
	auto& regs = vdp.registers();
	auto& render = vdp.render();

	regs.R16.H = 0b00;

	regs.R16.W = 0b00; // 32 tiles width
	auto row = get_plane_row(vdp, plane, 0);
	ASSERT_EQ(32 * 8, row.size());
	ASSERT_EQ(32 * 8, render.plane_width_in_pixels(plane));

	regs.R16.W = 0b01; // 128 tiles width
	row = get_plane_row(vdp, plane, 0);
	ASSERT_EQ(64 * 8, row.size());
	ASSERT_EQ(64 * 8, render.plane_width_in_pixels(plane));

	regs.R16.W = 0b11; // 128 tiles width
	row = get_plane_row(vdp, plane, 0);
	ASSERT_EQ(128 * 8, row.size());
	ASSERT_EQ(128 * 8, render.plane_width_in_pixels(plane));
}

TEST(VDP_RENDER, PLANE_W_ROW_WIDTH)
{
	vdp vdp;
	auto& regs = vdp.registers();
	auto& render = vdp.render();

	// TODO: take into accout RS1?
	regs.R12.RS0 = 1;
	auto row = get_plane_row(vdp, plane_type::w, 0);
	ASSERT_EQ(64 * 8, row.size());
	ASSERT_EQ(64 * 8, render.plane_width_in_pixels(plane_type::w));

	// TODO: take into accout RS1?
	regs.R12.RS0 = 0;
	row = get_plane_row(vdp, plane_type::w, 0);
	ASSERT_EQ(32 * 8, row.size());
	ASSERT_EQ(32 * 8, render.plane_width_in_pixels(plane_type::w));
}

TEST(VDP_RENDER, PLANE_AB_HEIGHT)
{
	vdp vdp;
	const auto plane = random::pick({plane_type::a, plane_type::b});
	auto& regs = vdp.registers();
	auto& render = vdp.render();

	regs.R16.W = 0b00;

	regs.R16.H = 0b00; // 32 tiles width
	ASSERT_EQ(32 * 8, render.plane_height_in_pixels(plane));

	regs.R16.H = 0b01; // 128 tiles width
	ASSERT_EQ(64 * 8, render.plane_height_in_pixels(plane));

	regs.R16.H = 0b11; // 128 tiles width
	ASSERT_EQ(128 * 8, render.plane_height_in_pixels(plane));
}

TEST(VDP_RENDER, PLANE_W_HEIGHT)
{
	vdp vdp;
	auto& regs = vdp.registers();
	auto& render = vdp.render();

	// TODO: take into accout RS1?
	regs.R12.RS0 = 1;
	ASSERT_EQ(32 * 8, render.plane_height_in_pixels(plane_type::w));

	// TODO: take into accout RS1?
	regs.R12.RS0 = 0;
	ASSERT_EQ(32 * 8, render.plane_height_in_pixels(plane_type::w));
}

TEST(VDP_RENDER, PLANE_DRAW_SAME_TAIL)
{
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

	vdp vdp;
	setup_and_run_plane_test(vdp, false, false, tail, [&](int row, int col, std::uint8_t palette)
	{
		auto tail_idx = get_tail_index(row % 8, col % 8);
		return read_color(vdp, palette, tail.at(tail_idx));
	});
}

TEST(VDP_RENDER, PLANE_HORIZONTAL_FLIP_TAIL)
{
	vdp vdp;
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
	auto tail = random_tail();

	setup_and_run_plane_test(vdp, false, true, tail, [&](int row, int col, std::uint8_t palette)
	{
		auto tail_idx = get_tail_index(row % 8, col % 8, false, true);
		return read_color(vdp, palette, tail.at(tail_idx));
	});
}

TEST(VDP_RENDER, PLANE_VERTICAL_AND_HORIZONTAL_FLIP_TAIL)
{
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

	vdp vdp;
	setup_and_run_plane_test(vdp, true, true, tail, [&](int row, int col, std::uint8_t palette)
	{
		auto tail_idx = get_tail_index(row % 8, col % 8);
		return read_color(vdp, palette, fliped_tail.at(tail_idx));
	});
}

TEST(VDP_RENDER, PLANE_ROW_UNIQUE_TILES_PER_ROW)
{
	// Setup
	vdp vdp;
	auto& vram = vdp.vram();
	auto& render = vdp.render();

	auto plane_type = random_plane();

	set_plane_dimension(vdp, plane_type);

	std::vector<std::vector<std::uint8_t>> tails;
	for(unsigned i = 0; i < render.plane_height_in_pixels(plane_type) / 8; ++i)
		tails.push_back(random_tail());
	ASSERT_TRUE(tails.size() > 0 && tails.size() <= 128);

	const std::uint32_t tail_base_address = 0b0001000000000000;
	auto tail_addr = tail_base_address;
	std::vector<std::uint32_t> tail_addresses;
	for(const auto& tail : tails)
	{
		copy_tail(vdp, tail_addr, tail);
		tail_addresses.push_back(tail_addr);
		tail_addr += 32; // each tail is 32 bytes long (8*8*4bits)
	}
	ASSERT_TRUE(tails.size() == tail_addresses.size());

	std::uint32_t plane_address = set_plane_address(vdp, plane_type, 0b100);

	auto palette = random_palette();
	for(auto address : tail_addresses)
	{
		auto plane_entry = get_plane_entry(address, false, false, palette);
		for(unsigned i = 0; i < render.plane_width_in_pixels(plane_type) / 8; ++i)
		{
			vram.write(plane_address, plane_entry);
			plane_address += sizeof(plane_entry);
		}
	}

	fill_cram(vdp);

	// Assert
	for(unsigned tail_row = 0; tail_row < render.plane_height_in_pixels(plane_type) / 8; ++tail_row)
	{
		const auto& tail = tails.at(tail_row);
		for(int pixel_row = 0; pixel_row < 8; ++pixel_row)
		{
			auto row_number = (tail_row * 8) + pixel_row;
			auto row = get_plane_row(vdp, plane_type, row_number);

			int col_idx = 0;
			for(auto actual_color : row)
			{
				auto tail_idx = get_tail_index(row_number % 8, col_idx % 8);

				genesis::vdp::output_color expected_color = read_color(vdp, palette, tail.at(tail_idx));

				ASSERT_EQ(expected_color, actual_color)
					<< "row: " << row_number << ", col: " << col_idx
					<< ", color id: " << (int)tail.at(tail_idx);

				++col_idx;
			}
		}
	}
}

TEST(VDP_RENDER, PLANE_ROW_INCORRECT_ROW_NUMBER_MUST_THROW)
{
	vdp vdp;
	renderer_builder builder(vdp);

	auto tail = random_tail();
	auto plane = random_plane();
	
	builder.setup_plane(plane, tail);

	auto& render = vdp.render();

	// Max possible resolution is 128 tails, i.e. 1024 rows
	// as it's 0-based, 1024 is always invalid row number
	ASSERT_THROW(get_plane_row(vdp, plane, 1024), std::exception);

	const unsigned last_valid_row = render.plane_height_in_pixels(plane) - 1;

	ASSERT_NO_THROW(get_plane_row(vdp, plane, last_valid_row));
	ASSERT_THROW(get_plane_row(vdp, plane, last_valid_row + 1), std::exception);
}

TEST(VDP_RENDERER, RENDER_TRANSPARENT_FRAMES)
{
	// Setup
	vdp vdp;
	renderer_builder builder(vdp);

	// Act
	builder.reset();

	// Assert
	run_active_display_test(vdp, [&](int /* row */, int /* col */)
	{
		return vdp.render().background_color();
	});
}

TEST(VDP_RENDERER, ACTIVE_AB_PLANE_DRAW_TAIL)
{
	vdp vdp;
	for(auto plane : {plane_type::a, plane_type::b})
	{
		auto other_plane = plane == plane_type::a ? plane_type::b : plane_type::a;

		renderer_builder builder(vdp);

		// Setup planes
		auto tail_main = random_tail();
		auto tail_trans = transparent_tail();

		bool hflip = random::is_true();
		bool vflip = random::is_true();
		std::uint8_t palette = random_palette();

		// set priority to false for the main plane and to true for the other
		// as second plane is transparent, priority flag should not affect the result

		// setup main plane to random tail
		builder.setup_plane(plane, tail_main, hflip, vflip, palette, false);

		// and the other to transparent tail
		builder.setup_plane(other_plane, tail_trans, hflip, vflip, random_palette(), true);

		builder.setup_plane(plane_type::w, tail_trans);

		fill_cram(vdp);

		// Assert
		run_active_display_test(vdp, [&](int row, int col)
		{
			auto tail_idx = get_tail_index(row % 8, col % 8, hflip, vflip);
			return read_active_color(vdp, palette, tail_main.at(tail_idx));
		});

		if(testing::Test::HasFatalFailure())
		{
			FAIL() << "Failed plane: " << static_cast<int>(plane) << '\n';
			break;
		}
	}
}

TEST(VDP_RENDERER, ACTIVE_W_PLANE_DRAW_TAIL)
{
	vdp vdp;
	renderer_builder builder(vdp);

	// Setup planes
	auto tail_main = random_tail(0x1, 0xF); // do not use transparent pixels

	bool hflip = random::is_true();
	bool vflip = random::is_true();
	std::uint8_t palette = random_palette();

	// setup main plane to random tail
	builder.setup_plane(plane_type::w, tail_main, hflip, vflip, palette);

	auto& regs = vdp.registers();
	regs.R17.HP = 0; // from the 0th column
	regs.R17.R = 1;  // to the last one
	regs.R18.D = 1;  // from the 0th line
	regs.R18.VP = 0; // to the last one

	// use random tails for A/B planes as they should be overlaped by W plane anyway
	builder.setup_plane(plane_type::a, random_tail(), hflip, vflip, random_palette());
	builder.setup_plane(plane_type::b, random_tail(), hflip, vflip, random_palette());

	fill_cram(vdp);

	// Assert
	run_active_display_test(vdp, [&](int row, int col)
	{
		auto tail_idx = get_tail_index(row % 8, col % 8, hflip, vflip);
		return read_active_color(vdp, palette, tail_main.at(tail_idx));
	});
}

TEST(VDP_RENDERER, ACTIVE_FULL_SCREEN_H_SCROLLING)
{
	vdp vdp;
	for(auto plane : {plane_type::a, plane_type::b})
	{
		auto other_plane = plane == plane_type::a ? plane_type::b : plane_type::a;

		for(std::uint16_t hscroll : {1, 2, 0xFFFE, 0xFFFF, 8, 16, (int)random::next<std::uint16_t>()})
		{
			renderer_builder builder(vdp);

			// Setup planes
			auto tail_main = random_tail();
			auto tail_trans = transparent_tail();

			bool hflip = random::is_true();
			bool vflip = random::is_true();

			builder.setup_plane(plane, tail_main, hflip, vflip);
			builder.setup_plane(other_plane, tail_trans);

			fill_cram(vdp);

			// Act
			if(plane == plane_type::a)
				builder.set_screen_hscroll(hscroll, random::next<std::uint16_t>());
			else
				builder.set_screen_hscroll(random::next<std::uint16_t>(), hscroll);

			// Assert
			run_active_display_test(vdp, [&](int row, int col)
			{
				auto tail_idx = get_tail_index(row % 8, col % 8, hflip, vflip, hscroll);
				return read_active_color(vdp, 0, tail_main.at(tail_idx));
			});

			if(testing::Test::HasFatalFailure())
			{
				FAIL() << "Failed plane: " << static_cast<int>(plane)
					<< ", hscroll: " << hscroll << '\n';
				return;
			}
		}
	}
}

TEST(VDP_RENDERER, ACTIVE_LINE_H_SCROLLING)
{
	vdp vdp;
	for(auto plane : {plane_type::a, plane_type::b})
	{
		auto other_plane = plane == plane_type::a ? plane_type::b : plane_type::a;

		// we can have up to 240 lines per display
		std::vector<std::uint16_t> scroll_main = random::next_few<std::uint16_t>(240);
		std::vector<std::uint16_t> scroll_other = random::next_few<std::uint16_t>(240);

		renderer_builder builder(vdp);

		// Setup planes
		auto tail_main = random_tail();
		auto tail_trans = transparent_tail();

		bool hflip = random::is_true();
		bool vflip = random::is_true();

		builder.setup_plane(plane, tail_main, hflip, vflip);
		builder.setup_plane(other_plane, tail_trans);

		fill_cram(vdp);

		// Act
		if(plane == plane_type::a)
			builder.set_line_hscroll(scroll_main, scroll_other);
		else
			builder.set_line_hscroll(scroll_other, scroll_main);

		// Assert
		run_active_display_test(vdp, [&](int row, int col)
		{
			auto tail_idx = get_tail_index(row % 8, col % 8, hflip, vflip, scroll_main.at(row));
			return read_active_color(vdp, 0, tail_main.at(tail_idx));
		});

		if(testing::Test::HasFatalFailure())
		{
			FAIL() << "Failed plane: " << static_cast<int>(plane) << '\n';
			return;
		}
	}
}
