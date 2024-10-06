#include "vdp/impl/hv_counters.h"

#include <gtest/gtest.h>
#include <iostream>

using namespace genesis::vdp;

template <class counter_t, class inc_counter_fn>
void test_counter(int min1, int max1, int min2, int max2, int min3, int max3, counter_t& counter,
				  inc_counter_fn inc_counter)
{
	for(int i = 0; i < 1; ++i)
	{
		int raw_counter = 0;
		for(int expected_value = min1; expected_value <= max1; ++expected_value)
		{
			// std::cout << "Counter value: " << (int)get_value() << "\n";
			ASSERT_EQ(raw_counter++, counter.raw_value());
			ASSERT_EQ(expected_value, counter.value());
			inc_counter();
		}

		for(int expected_value = min2; expected_value <= max2; ++expected_value)
		{
			// std::cout << "Counter value: " << (int)get_value() << "\n";
			ASSERT_EQ(raw_counter++, counter.raw_value());
			ASSERT_EQ(expected_value, counter.value());
			inc_counter();
		}

		for(int expected_value = min3; expected_value <= max3; ++expected_value)
		{
			// std::cout << "Counter value: " << (int)get_value() << "\n";
			ASSERT_EQ(raw_counter++, counter.raw_value());
			ASSERT_EQ(expected_value, counter.value());
			inc_counter();
		}
	}
}

template <class counter_t, class inc_counter_fn>
void test_counter(int min1, int max1, int min2, int max2, counter_t& counter, inc_counter_fn inc_counter)
{
	test_counter(min1, max1, min2, max2, 1, 0, counter, inc_counter);
}

TEST(VDP_H_COUNTER, H32)
{
	impl::h_counter counter;

	test_counter(0x00, 0x93, 0xE9, 0xFF, counter, [&counter]() { return counter.inc(display_width::c32); });
}

TEST(VDP_H_COUNTER, H40)
{
	impl::h_counter counter;

	test_counter(0x00, 0xB6, 0xE4, 0xFF, counter, [&counter]() { return counter.inc(display_width::c40); });
}

TEST(VDP_V_COUNTER, PAL_V28)
{
	impl::v_counter counter;

	test_counter(0x00, 0xFF, 0x00, 0x02, 0xCA, 0xFF, counter,
				 [&counter]() { return counter.inc(display_height::c28, mode::PAL); });
}

TEST(VDP_V_COUNTER, PAL_V30)
{
	impl::v_counter counter;

	test_counter(0x00, 0xFF, 0x00, 0x0A, 0xD2, 0xFF, counter,
				 [&counter]() { return counter.inc(display_height::c30, mode::PAL); });
}

TEST(VDP_V_COUNTER, NTSC_V28)
{
	impl::v_counter counter;

	test_counter(0x00, 0xEA, 0xE5, 0xFF, counter,
				 [&counter]() { return counter.inc(display_height::c28, mode::NTSC); });
}

TEST(VDP_V_COUNTER, NTSC_V30)
{
	impl::v_counter counter;

	test_counter(0x00, 0xFF, 0x00, 0xFF, counter,
				 [&counter]() { return counter.inc(display_height::c30, mode::NTSC); });
}
