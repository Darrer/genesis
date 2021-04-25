#include "memory.hpp"

#include <cstdint>
#include <gtest/gtest.h>


using mem_addr = std::uint16_t;
using big_end_memory = genesis::memory<mem_addr, 0xff, std::endian::big>;
using little_end_memory = genesis::memory<mem_addr, 0xff, std::endian::little>;


TEST(Memory, WrongAccess)
{
	auto mem = big_end_memory();

	mem_addr test_addr = 0x1f1;
	ASSERT_THROW(mem.read<std::byte>(test_addr), std::runtime_error);
	ASSERT_THROW(mem.write<std::byte>(test_addr, std::byte(0x0)), std::runtime_error);

	test_addr = 0xfe;
	ASSERT_THROW(mem.read<std::uint16_t>(test_addr), std::runtime_error);
	ASSERT_THROW(mem.write<std::uint16_t>(test_addr, std::uint16_t(0xf)), std::runtime_error);
}


TEST(Memory, ReadWrite)
{
	auto test_read_write = [](auto mem, auto addr, auto data) {
		mem.write(addr, data);

		auto new_data = mem.template read<decltype(data)>(addr);

		ASSERT_EQ(data, new_data);
	};

	auto test_mem = [&test_read_write](auto mem) {
		mem_addr addr = 0x0;
		for (size_t i = 0; i < 10; ++i)
		{
			test_read_write(mem, addr, std::uint8_t(0xAF));
			test_read_write(mem, addr, std::uint16_t(0xAF91));

			addr = i * 10;
		}
	};

	auto mem1 = little_end_memory();
	auto mem2 = big_end_memory();

	test_mem(mem1);
	test_mem(mem2);
}
