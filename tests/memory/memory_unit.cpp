#include "memory/memory_unit.h"

#include "../helpers/random.h"
#include "helper.h"

#include <gtest/gtest.h>

using namespace genesis;


TEST(MEMORY, MEMORY_UNIT_CAPACITY)
{
	const std::uint32_t highest_address = 1024;
	memory::memory_unit unit{highest_address};

	const std::uint32_t expected_capacity = highest_address + 1;
	ASSERT_EQ(expected_capacity, unit.capacity());
}

TEST(MEMORY, MEMORY_UNIT_MAX_ADDRESS)
{
	const std::uint32_t highest_address = 1024;
	memory::memory_unit unit{highest_address};

	ASSERT_EQ(highest_address, unit.max_address());
}

TEST(MEMORY, MEMORY_UNIT_IDLE)
{
	memory::memory_unit unit{123};

	unit.init_read_byte(0);
	ASSERT_TRUE(unit.is_idle());

	unit.init_write(0, std::uint16_t(0));
	ASSERT_TRUE(unit.is_idle());
}

TEST(MEMORY, MEMORY_UNIT_WRITE_8_BOUNDARIES)
{
	const std::uint32_t highest_address = 256;
	memory::memory_unit unit{highest_address};

	ASSERT_NO_THROW(unit.write<std::uint8_t>(0, 0));
	ASSERT_NO_THROW(unit.write<std::uint8_t>(highest_address, 0));

	ASSERT_THROW(unit.write<std::uint8_t>(highest_address + 1, 0), std::runtime_error);
}

TEST(MEMORY, MEMORY_UNIT_WRITE_16_BOUNDARIES)
{
	const std::uint32_t highest_address = 256;
	memory::memory_unit unit{highest_address};

	ASSERT_NO_THROW(unit.write<std::uint16_t>(0, 0));
	ASSERT_NO_THROW(unit.write<std::uint16_t>(highest_address - 1, 0));

	ASSERT_THROW(unit.write<std::uint16_t>(highest_address, 0), std::runtime_error);
	ASSERT_THROW(unit.write<std::uint16_t>(highest_address + 1, 0), std::runtime_error);
}

TEST(MEMORY, MEMORY_UNIT_READ_8_BOUNDARIES)
{
	const std::uint32_t highest_address = 256;
	memory::memory_unit unit{highest_address};

	ASSERT_NO_THROW(unit.init_read_byte(0));
	ASSERT_NO_THROW(unit.init_read_byte(highest_address));

	ASSERT_THROW(unit.init_read_byte(highest_address + 1), std::runtime_error);
}

TEST(MEMORY, MEMORY_UNIT_READ_16_BOUNDARIES)
{
	const std::uint32_t highest_address = 256;
	memory::memory_unit unit{highest_address};

	ASSERT_NO_THROW(unit.init_read_word(0));
	ASSERT_NO_THROW(unit.init_read_word(highest_address - 1));

	ASSERT_THROW(unit.init_read_word(highest_address), std::runtime_error);
	ASSERT_THROW(unit.init_read_word(highest_address + 1), std::runtime_error);
}

TEST(MEMORY, MEMORY_UNIT_READ_WRITE_8)
{
	memory::memory_unit unit{256};
	test::test_read_write<std::uint8_t>(unit);
}

TEST(MEMORY, MEMORY_UNIT_READ_WRITE_16)
{
	memory::memory_unit unit_le{256, std::endian::little};
	test::test_read_write<std::uint16_t>(unit_le);

	memory::memory_unit unit_be{256, std::endian::big};
	test::test_read_write<std::uint16_t>(unit_be);
}

TEST(MEMORY, MEMORY_UNIT_BIG_ENDIAN)
{
	const int num_batches = 10;
	memory::memory_unit unit{32, std::endian::big};

	for(int i = 0; i < num_batches; ++i)
	{
		std::uint16_t data = test::random::next<std::uint16_t>();

		unit.write(0, data);

		std::uint8_t first = unit.read<std::uint8_t>(0);
		std::uint8_t second = unit.read<std::uint8_t>(1);

		std::uint8_t msb = endian::msb(data);
		std::uint8_t lsb = endian::lsb(data);

		ASSERT_EQ(msb, first);
		ASSERT_EQ(lsb, second);
	}
}

TEST(MEMORY, MEMORY_UNIT_LITTLE_ENDIAN)
{
	const int num_batches = 10;
	memory::memory_unit unit{32, std::endian::little};

	for(int i = 0; i < num_batches; ++i)
	{
		std::uint16_t data = test::random::next<std::uint16_t>();

		unit.write(0, data);

		std::uint8_t first = unit.read<std::uint8_t>(0);
		std::uint8_t second = unit.read<std::uint8_t>(1);

		std::uint8_t msb = endian::msb(data);
		std::uint8_t lsb = endian::lsb(data);

		ASSERT_EQ(lsb, first);
		ASSERT_EQ(msb, second);
	}
}

TEST(MEMORY, MEMORY_UNIT_LATCHED_DATA_WITHOUT_READ)
{
	memory::memory_unit unit{32};

	ASSERT_THROW(unit.latched_byte(), std::exception);
	ASSERT_THROW(unit.latched_word(), std::exception);
}

TEST(MEMORY, MEMORY_UNIT_LATCHED_DATA_AFTER_WRITE_8)
{
	memory::memory_unit unit{32};

	unit.init_read_byte(0);
	unit.init_write(0, std::uint8_t(0));

	ASSERT_THROW(unit.latched_byte(), std::exception);
	ASSERT_THROW(unit.latched_word(), std::exception);
}

TEST(MEMORY, MEMORY_UNIT_LATCHED_DATA_AFTER_WRITE_16)
{
	memory::memory_unit unit{32};

	unit.init_read_word(0);
	unit.init_write(0, std::uint16_t(0));

	ASSERT_THROW(unit.latched_byte(), std::exception);
	ASSERT_THROW(unit.latched_word(), std::exception);
}

TEST(MEMORY, MEMORY_UNIT_LATCHED_BYTE_AFTER_READING_WORD)
{
	memory::memory_unit unit{32};

	unit.init_read_word(0);

	ASSERT_THROW(unit.latched_byte(), std::exception);
}

TEST(MEMORY, MEMORY_UNIT_LATCHED_WORD_AFTER_READING_BYTE)
{
	memory::memory_unit unit{32};

	unit.init_read_byte(0);

	ASSERT_THROW(unit.latched_word(), std::exception);
}
