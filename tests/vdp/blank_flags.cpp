#include "vdp/impl/blank_flags.h"

#include <gtest/gtest.h>

using namespace genesis::vdp;

template <class flag_t, class update_func>
void test_flag(flag_t& flag, int set_value, int clear_value, update_func update)
{
	ASSERT_FALSE(flag.value());

	bool expected_value = false;
	for(int i = 0; i < 5; ++i)
	{
		for(int counter = 0; counter < 1024; ++counter)
		{
			if(counter == set_value)
				expected_value = true;
			else if(counter == clear_value)
				expected_value = false;

			update(counter);
			ASSERT_EQ(expected_value, flag.value()) << "counter: " << counter;
		}
	}
}

TEST(VDP_HBLANK_FLAG, H32)
{
	impl::hblank_flag flag;

	test_flag(flag, 0x93, 0x05, [&flag](int counter) { flag.update(counter, display_width::c32); });
}

TEST(VDP_HBLANK_FLAG, H40)
{
	impl::hblank_flag flag;

	test_flag(flag, 0xB3, 0x06, [&flag](int counter) { flag.update(counter, display_width::c40); });
}

TEST(VDP_VBLANK_FLAG, PAL_V28)
{
	impl::vblank_flag flag;

	test_flag(flag, 0xE0, 0x138, [&flag](int counter) { flag.update(counter, display_height::c28, mode::PAL); });
}

TEST(VDP_VBLANK_FLAG, PAL_V30)
{
	impl::vblank_flag flag;

	test_flag(flag, 0xF0, 0x138, [&flag](int counter) { flag.update(counter, display_height::c30, mode::PAL); });
}

TEST(VDP_VBLANK_FLAG, NTSC_V28)
{
	impl::vblank_flag flag;

	test_flag(flag, 0xE0, 0x105, [&flag](int counter) { flag.update(counter, display_height::c28, mode::NTSC); });
}

TEST(VDP_VBLANK_FLAG, NTSC_V30)
{
	impl::vblank_flag flag;

	test_flag(flag, 0xF0, 0x1FF, [&flag](int counter) { flag.update(counter, display_height::c30, mode::NTSC); });
}
