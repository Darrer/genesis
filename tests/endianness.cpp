#include <gtest/gtest.h>

#include "endianness.hpp"


TEST(endianness, uint32_type)
{
	uint32_t val = 0x12345678;

	endianness::swap(val);

	EXPECT_EQ(val, 0x78563412);
}


TEST(endianness, uint16_type)
{
	uint16_t val = 0x1234;

	endianness::swap(val);

	EXPECT_EQ(val, 0x3412);
}
