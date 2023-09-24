#include <gtest/gtest.h>

#include "memory/memory_builder.h"
#include "memory/memory_unit.h"
#include "helper.h"

using namespace genesis;

std::shared_ptr<memory::memory_unit> device(std::uint32_t highest_address)
{
	return std::make_shared<memory::memory_unit>(highest_address);
}

std::shared_ptr<memory::addressable> build(unsigned num_devices, unsigned mem_per_device)
{
	memory::memory_builder builder;

	std::uint32_t start_address = 0;
	for(unsigned i = 0; i < num_devices; ++i)
	{
		builder.add(device(mem_per_device - 1), start_address);
		start_address += mem_per_device;
	}

	return builder.build();
}

TEST(MEMORY, MEMORY_BUILDER_INTERSECT_DEVICES)
{
	memory::memory_builder builder;

	auto dev = device(1024);
	builder.add(dev, 1024);

	/* check start address intersection */
	ASSERT_THROW(builder.add(dev, 1024), std::exception); // check low boundary
	ASSERT_THROW(builder.add(dev, 2048), std::exception); // check upper boundary
	ASSERT_THROW(builder.add(dev, 1536), std::exception); // check in between

	/* check end address intersection */
	ASSERT_THROW(builder.add(dev, 0), std::exception); // check low boundary
	ASSERT_THROW(builder.add(dev, 1023), std::exception); // check upper boundary
	ASSERT_THROW(builder.add(dev, 512), std::exception); // check in between
}

TEST(MEMORY, MEMORY_BUILDER_READ_WRITE_8)
{
	auto composite_memory = build(10, 128);
	test::test_read_write<std::uint8_t>(*composite_memory);
}

TEST(MEMORY, MEMORY_BUILDER_READ_WRITE_16)
{
	auto composite_memory = build(10, 128);
	test::test_read_write<std::uint16_t>(*composite_memory);
}

TEST(MEMORY, MEMORY_BUILDER_GAP)
{
	memory::memory_builder builder;

	builder.add(device(32), 0); // [0 ; 32]
	builder.add(device(32), 1024); // [1024 ; 1056]

	auto mem = builder.build();

	for(auto addr : {33, 512, 1057})
	{
		ASSERT_THROW(mem->init_write(addr, std::uint8_t(0)), std::runtime_error);
		ASSERT_THROW(mem->init_write(addr, std::uint16_t(0)), std::runtime_error);

		ASSERT_THROW(mem->init_read_byte(addr), std::runtime_error);
		ASSERT_THROW(mem->init_read_word(addr), std::runtime_error);
	}
}

TEST(MEMORY, MEMORY_BUILDER_UNITS_BORDER)
{
	memory::memory_builder builder;

	builder.add(device(32), 0); // [0 ; 32]
	builder.add(device(32), 33); // [33 ; 65]

	const std::uint32_t border_address = 32;

	auto mem = builder.build();

	// Check if read/write operation is requested right on the border between 2 memory units
	// So far assume exception is thrown
	ASSERT_THROW(mem->init_read_word(border_address), std::runtime_error);
	ASSERT_THROW(mem->init_write(border_address, std::uint16_t(0)), std::runtime_error);

	ASSERT_NO_THROW(mem->init_read_byte(border_address));
	ASSERT_NO_THROW(mem->init_write(border_address, std::uint8_t(0)));
}

TEST(MEMORY, MEMORY_BUILDER_EMPTY_BUILD)
{
	memory::memory_builder builder;

	ASSERT_THROW(builder.build(), std::runtime_error);
}

TEST(MEMORY, MEMORY_BUILDER_CLEAR_AFTER_BUILD)
{
	memory::memory_builder builder;

	builder.add(device(32), 0);
	builder.add(device(32), 1024);

	ASSERT_NO_THROW(builder.build());
	ASSERT_THROW(builder.build(), std::runtime_error); // second build call must throw
}
