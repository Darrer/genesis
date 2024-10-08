#include "memory/memory_builder.h"

#include "helper.h"
#include "memory/memory_unit.h"

#include <gtest/gtest.h>

using namespace genesis;

std::shared_ptr<memory::memory_unit> shared_device(std::uint32_t highest_address)
{
	return std::make_shared<memory::memory_unit>(highest_address);
}

std::unique_ptr<memory::addressable> unique_device(std::uint32_t highest_address)
{
	return std::make_unique<memory::memory_unit>(highest_address);
}

std::shared_ptr<memory::addressable> build(unsigned num_devices, unsigned mem_per_device)
{
	memory::memory_builder builder;

	std::uint32_t start_address = 0;
	for(unsigned i = 0; i < num_devices; ++i)
	{
		if(i % 2 == 0)
		{
			builder.add(shared_device(mem_per_device - 1), start_address);
		}
		else
		{
			builder.add(unique_device(mem_per_device - 1), start_address);
		}

		start_address += mem_per_device;
	}

	return builder.build();
}

TEST(MEMORY, MEMORY_BUILDER_INTERSECT_DEVICES)
{
	auto test_boundaries = [](auto make_device) {
		memory::memory_builder builder;
		builder.add(make_device(1024), 1024);

		auto dev = shared_device(1024);

		/* check start address intersection */
		ASSERT_THROW(builder.add(dev, 1024), std::exception); // check low boundary
		ASSERT_THROW(builder.add(dev, 2048), std::exception); // check upper boundary
		ASSERT_THROW(builder.add(dev, 1536), std::exception); // check in between

		/* check end address intersection */
		ASSERT_THROW(builder.add(dev, 0), std::exception);	  // check low boundary
		ASSERT_THROW(builder.add(dev, 1023), std::exception); // check upper boundary
		ASSERT_THROW(builder.add(dev, 512), std::exception);  // check in between
	};

	test_boundaries(shared_device);
	test_boundaries(unique_device);
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

	builder.add(shared_device(32), 0);	  // [0 ; 32]
	builder.add(unique_device(32), 1024); // [1024 ; 1056]

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

	builder.add(shared_device(32), 0);	// [0 ; 32]
	builder.add(unique_device(32), 33); // [33 ; 65]

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

	builder.add(shared_device(32), 0);
	builder.add(unique_device(32), 1024);

	ASSERT_NO_THROW(builder.build());
	ASSERT_THROW(builder.build(), std::runtime_error); // second build call must throw
}

TEST(MEMORY, MEMORY_BUILDER_MIRROR_INVALID_ADDRESSES)
{
	memory::memory_builder builder;

	builder.add(shared_device(0x20), 0);	// [0 ; 0x20]
	builder.add(unique_device(0x20), 0x21); // [0x21 ; 0x41]

	// different ranges capacity
	ASSERT_THROW(builder.mirror(0x0, 0x2, 0x21, 0x23), std::exception);

	// ranges belong to the same existing device
	ASSERT_THROW(builder.mirror(0x0, 0x2, 0x0, 0x2), std::exception);
	ASSERT_THROW(builder.mirror(0x0, 0x2, 0x2, 0x4), std::exception);
	ASSERT_THROW(builder.mirror(0x0, 0x2, 0x3, 0x5), std::exception);

	// initial address range occupies 2 devices
	ASSERT_THROW(builder.mirror(0x20, 0x21, 0x50, 0x51), std::exception);

	// initial address range is unknown
	ASSERT_THROW(builder.mirror(0x45, 0x46, 0x50, 0x51), std::exception);

	// try to map on existing device
	ASSERT_THROW(builder.mirror(0x0, 0x2, 0x21, 0x23), std::exception);
}

TEST(MEMORY, MEMORY_BUILDER_MIRROR)
{
	memory::memory_builder builder;

	builder.add(shared_device(0x20), 0);	// [0 ; 0x20]
	builder.add(unique_device(0x20), 0x21); // [0x21 ; 0x41]

	const std::uint32_t START_MIRROR = 0x50;
	const std::uint32_t END_MIRROR = 0x70;

	builder.mirror(0x0, 0x20, START_MIRROR, END_MIRROR);
	auto mem = builder.build();

	for(std::uint32_t addr = START_MIRROR; addr <= END_MIRROR; ++addr)
	{
		std::uint8_t data = test::random::next<std::uint8_t>();
		mem->init_write(addr, data);

		mem->init_read_byte(addr - START_MIRROR);
		ASSERT_EQ(data, mem->latched_byte());
	}
}
