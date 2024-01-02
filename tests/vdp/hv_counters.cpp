#include <gtest/gtest.h>

#include <iostream>

#include "vdp/impl/hv_counters.h"

using namespace genesis::vdp;

template<class get_value_fn, class inc_counter_fn>
void test_counter(int min1, int max1, int min2, int max2, int min3, int max3,
	get_value_fn get_value, inc_counter_fn inc_counter)
{
	for(int i = 0; i < 2; ++i)
	{
		for(int expected_value = min1; expected_value <= max1; ++expected_value)
		{
			// std::cout << "Counter value: " << (int)get_value() << "\n";
			ASSERT_EQ(expected_value, get_value());
			inc_counter();
		}
		
		for(int expected_value = min2; expected_value <= max2; ++expected_value)
		{
			// std::cout << "Counter value: " << (int)get_value() << "\n";
			ASSERT_EQ(expected_value, get_value());
			inc_counter();
		}

		for(int expected_value = min3; expected_value <= max3; ++expected_value)
		{
			// std::cout << "Counter value: " << (int)get_value() << "\n";
			ASSERT_EQ(expected_value, get_value());
			inc_counter();
		}
	}
}

template<class get_value_fn, class inc_counter_fn>
void test_counter(int min1, int max1, int min2, int max2, get_value_fn get_value, inc_counter_fn inc_counter)
{
	test_counter(min1, max1, min2, max2, 1, 0, get_value, inc_counter);
}

TEST(VDP_H_COUNTER, H32)
{
	impl::h_counter counter;

	test_counter(0x00, 0x93, 0xE9, 0xFF,
		[&counter](){ return counter.value(); },
		[&counter](){ return counter.inc(display_width::c32); });
}

TEST(VDP_H_COUNTER, H40)
{
	impl::h_counter counter;

	test_counter(0x00, 0xB6, 0xE4, 0xFF,
		[&counter](){ return counter.value(); },
		[&counter](){ return counter.inc(display_width::c40); });
}

TEST(VDP_V_COUNTER, PAL_V28)
{
	impl::v_counter counter;

	test_counter(0x00, 0xFF, 0x00, 0x02, 0xCA, 0xFF,
		[&counter]() { return counter.value(); },
		[&counter]() { return counter.inc(display_height::c28, true); });
}

TEST(VDP_V_COUNTER, PAL_V30)
{
	impl::v_counter counter;

	test_counter(0x00, 0xFF, 0x00, 0x0A, 0xD2, 0xFF,
		[&counter]() { return counter.value(); },
		[&counter]() { return counter.inc(display_height::c30, true); });
}

TEST(VDP_V_COUNTER, NTSC_V28)
{
	impl::v_counter counter;

	test_counter(0x00, 0xEA, 0xE5, 0xFF,
		[&counter]() { return counter.value(); },
		[&counter]() { return counter.inc(display_height::c28, false); });
}

TEST(VDP_V_COUNTER, NTSC_V30)
{
	impl::v_counter counter;

	test_counter(0x00, 0xFF, 0x00, 0xFF,
		[&counter]() { return counter.value(); },
		[&counter]() { return counter.inc(display_height::c30, false); });
}
