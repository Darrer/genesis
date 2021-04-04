#include <gtest/gtest.h>

#include "endianness.hpp"


TEST(endianness, uint32_type)
{
	uint8_t data[] = {0x1, 0x2, 0x3, 0x4};
	uint32_t val = *((uint32_t*)data);

	endianness::swap(val);

	uint8_t swapped_data[4];
	swapped_data[0] = ((uint8_t*)(&val))[0];
	swapped_data[1] = ((uint8_t*)(&val))[1];
	swapped_data[2] = ((uint8_t*)(&val))[2];
	swapped_data[3] = ((uint8_t*)(&val))[3];

	EXPECT_EQ(swapped_data[0], 0x4);
	EXPECT_EQ(swapped_data[1], 0x3);
	EXPECT_EQ(swapped_data[2], 0x2);
	EXPECT_EQ(swapped_data[3], 0x1);
}


TEST(endianness, uint16_type)
{
	uint8_t data[] = {0x12, 0x34};
	uint16_t val = *((uint16_t*)data);

	endianness::swap(val);

	uint8_t swapped_data[2];
	swapped_data[0] = ((uint8_t*)(&val))[0];
	swapped_data[1] = ((uint8_t*)(&val))[1];

	EXPECT_EQ(swapped_data[0], 0x34);
	EXPECT_EQ(swapped_data[1], 0x12);
}
