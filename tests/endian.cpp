#include <gtest/gtest.h>

#include "endian.hpp"


TEST(Endian, uint32)
{
	uint32_t val = 0x12345678;

	endian::swap(val);

	ASSERT_EQ(val, 0x78563412);
}


TEST(Endian, uint16)
{
	uint16_t val = 0x1234;

	endian::swap(val);

	ASSERT_EQ(val, 0x3412);
}


TEST(Endian, ShouldNotSwap)
{
	if constexpr (std::endian::native == std::endian::little)
	{
		uint32_t val = 0x12345678;
		endian::little_to_sys(val);
		ASSERT_EQ(val, 0x12345678);

		val = 0x12345678;
		endian::sys_to_little(val);
		ASSERT_EQ(val, 0x12345678);
	}
	else if constexpr (std::endian::native == std::endian::big)
	{
		uint32_t val = 0x12345678;
		endian::big_to_sys(val);
		ASSERT_EQ(val, 0x12345678);

		val = 0x12345678;
		endian::sys_to_big(val);
		ASSERT_EQ(val, 0x12345678);
	}
}


TEST(Endian, ShouldSwap)
{
	if constexpr (std::endian::native == std::endian::little)
	{
		uint32_t val = 0x12345678;
		endian::big_to_sys(val);
		ASSERT_EQ(val, 0x78563412);

		val = 0x12345678;
		endian::sys_to_big(val);
		ASSERT_EQ(val, 0x78563412);
	}
	else if constexpr (std::endian::native == std::endian::big)
	{
		uint32_t val = 0x12345678;
		endian::little_to_sys(val);
		ASSERT_EQ(val, 0x78563412);

		val = 0x12345678;
		endian::sys_to_little(val);
		ASSERT_EQ(val, 0x78563412);
	}
}
