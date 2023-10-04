#include "test_cpu.hpp"

#include <array>
#include <gtest/gtest.h>

using namespace genesis;


void prepare_mem(memory::memory_unit& mem, std::uint32_t base_addr, const auto& array)
{
	for(auto val : array)
	{
		mem.write(base_addr, val);
		base_addr += sizeof(val);
	}
}

std::uint32_t wait_idle(m68k::bus_manager& busm)
{
	std::uint32_t cycles = 0;
	while(!busm.is_idle())
	{
		busm.cycle();
		++cycles;
	}

	return cycles;
}

std::uint32_t read_byte(m68k::bus_manager& busm, std::uint32_t addr)
{
	busm.init_read_byte(addr, m68k::addr_space::DATA);
	return wait_idle(busm);
}

std::uint32_t read_word(m68k::bus_manager& busm, std::uint32_t addr)
{
	busm.init_read_word(addr, m68k::addr_space::DATA);
	return wait_idle(busm);
}

template <class T>
std::uint32_t write(m68k::bus_manager& busm, std::uint32_t addr, T val)
{
	busm.init_write(addr, val);
	return wait_idle(busm);
}

std::array<std::uint8_t, 10> gen_test_bytes()
{
	return {0, 1, 255, 127, 126, 42, 24, 50, 100, 200};
}

std::array<std::uint16_t, 10> gen_test_words()
{
	return {0, 1, 255, 127, 126, 65535, 65534, 32768, 32767, 666};
}

template <class T, std::size_t N>
void test_read(std::array<T, N> test_values)
{
	test::test_cpu cpu;

	auto& busm = cpu.bus_manager();
	auto& mem = cpu.memory();

	std::uint32_t base = 0x100;
	prepare_mem(mem, base, test_values);

	const std::uint32_t cycles_per_read = 4;

	for(std::size_t i = 0; i < test_values.size(); ++i)
	{
		auto expected = test_values[i];
		std::uint32_t addr = base + i * sizeof(expected);

		std::uint32_t cycles = 0;
		decltype(expected) actual = 0;

		if constexpr(sizeof(expected) == 1)
		{
			cycles = read_byte(busm, addr);
			actual = busm.latched_byte();
		}
		else
		{
			cycles = read_word(busm, addr);
			actual = busm.latched_word();
		}

		ASSERT_EQ(cycles_per_read, cycles);
		ASSERT_EQ(expected, actual);
	}
}

template <class T, std::size_t N>
void test_write(std::array<T, N> values_to_write)
{
	test::test_cpu cpu;

	auto& busm = cpu.bus_manager();
	auto& mem = cpu.memory();

	const std::uint32_t cycles_per_write = 4;

	std::uint32_t base = 0x100;
	for(std::size_t i = 0; i < values_to_write.size(); ++i)
	{
		auto val = values_to_write[i];
		std::uint32_t addr = base + i * sizeof(val);

		auto cycles = write(busm, addr, val);

		ASSERT_EQ(cycles_per_write, cycles);
	}

	// check mem
	for(std::size_t i = 0; i < values_to_write.size(); ++i)
	{
		auto expected = values_to_write[i];
		std::uint32_t addr = base + i * sizeof(expected);
		auto actual = mem.read<decltype(expected)>(addr);

		ASSERT_EQ(expected, actual);
	}
}


TEST(M68K_BUS_MANAGER, READ_BYTE)
{
	test_read(gen_test_bytes());
}

TEST(M68K_BUS_MANAGER, READ_WORD)
{
	test_read(gen_test_words());
}

TEST(M68K_BUS_MANAGER, WRITE_BYTE)
{
	test_write(gen_test_bytes());
}

TEST(M68K_BUS_MANAGER, WRITE_WORD)
{
	test_write(gen_test_words());
}

TEST(M68K_BUS_MANAGER, INTERRUPT_CYCLE_THROW)
{
	test::test_cpu cpu;
	auto& busm = cpu.bus_manager();

	// assert initial state
	ASSERT_TRUE(busm.is_idle());

	busm.init_read_byte(0x100, m68k::addr_space::DATA);

	// should not allow to start new bus cycle while in the middle of the other
	ASSERT_THROW(busm.init_read_byte(0x101, m68k::addr_space::DATA), std::runtime_error);
	ASSERT_THROW(busm.init_read_word(0x100, m68k::addr_space::DATA), std::runtime_error);
	ASSERT_THROW(busm.init_write(0x100, (std::uint16_t)0x101), std::runtime_error);

	// should not allow to get latched data while in the middle of a cycle
	ASSERT_THROW(busm.latched_byte(), std::runtime_error);
	ASSERT_THROW(busm.latched_word(), std::runtime_error);
}

TEST(M68K_BUS_MANAGER, LETCH_WRONG_DATA_THROW)
{
	test::test_cpu cpu;
	auto& busm = cpu.bus_manager();

	read_byte(busm, 0x101);
	ASSERT_THROW(busm.latched_word(), std::runtime_error);

	read_word(busm, 0x102);
	ASSERT_THROW(busm.latched_byte(), std::runtime_error);
}

struct bus_state
{
	std::uint32_t address = 0;
	std::uint16_t data = 0;
	bool uds_is_set = false;
	bool lds_is_set = false;
};

template <class T>
bus_state read_and_track(std::uint32_t addr, T val_to_write)
{
	test::test_cpu cpu;

	auto& bus = cpu.bus();
	auto& busm = cpu.bus_manager();
	auto& mem = cpu.memory();

	mem.write(addr, val_to_write);

	if constexpr(sizeof(T) == 1)
		busm.init_read_byte(addr, m68k::addr_space::DATA);
	else
		busm.init_read_word(addr, m68k::addr_space::DATA);

	std::uint32_t cycle = 0;
	bus_state bs;

	while(!busm.is_idle())
	{
		busm.cycle();
		++cycle;

		if(cycle == 1)
			bs.address = bus.address();

		if(cycle == 2)
		{
			bs.uds_is_set = bus.is_set(m68k::bus::UDS);
			bs.lds_is_set = bus.is_set(m68k::bus::LDS);
		}

		if(cycle == 3)
			bs.data = bus.data();
	}

	return bs;
}

template <class T>
bus_state write_and_track(std::uint32_t addr, T val_to_write)
{
	test::test_cpu cpu;

	auto& bus = cpu.bus();
	auto& busm = cpu.bus_manager();

	busm.init_write(addr, val_to_write);

	std::uint32_t cycle = 0;
	bus_state bs;

	while(!busm.is_idle())
	{
		busm.cycle();
		++cycle;

		if(cycle == 1)
			bs.address = bus.address();

		if(cycle == 2)
			bs.data = bus.data();

		if(cycle == 3)
		{
			bs.uds_is_set = bus.is_set(m68k::bus::UDS);
			bs.lds_is_set = bus.is_set(m68k::bus::LDS);
		}
	}

	return bs;
}

TEST(M68K_BUS_MANAGER, READ_BYTE_BUS_TRANSITIONS)
{
	std::uint8_t data = 42;
	auto bs = read_and_track(0x101, data);

	ASSERT_EQ(0x101, bs.address);
	ASSERT_EQ(data, bs.data & 0xFF);
	ASSERT_FALSE(bs.uds_is_set);
	ASSERT_TRUE(bs.lds_is_set);

	bs = read_and_track(0x100, data);

	ASSERT_EQ(0x100, bs.address);
	ASSERT_EQ(data, bs.data >> 8);
	ASSERT_TRUE(bs.uds_is_set);
	ASSERT_FALSE(bs.lds_is_set);
}

TEST(M68K_BUS_MANAGER, WRITE_BYTE_BUS_TRANSITIONS)
{
	std::uint8_t data = 142;
	auto bs = write_and_track(0x101, data);

	ASSERT_EQ(0x101, bs.address);
	ASSERT_EQ(data, bs.data & 0xFF);
	ASSERT_FALSE(bs.uds_is_set);
	ASSERT_TRUE(bs.lds_is_set);

	bs = write_and_track(0x100, data);

	ASSERT_EQ(0x100, bs.address);
	ASSERT_EQ(data, bs.data >> 8);
	ASSERT_TRUE(bs.uds_is_set);
	ASSERT_FALSE(bs.lds_is_set);
}

TEST(M68K_BUS_MANAGER, READ_WORD_BUS_TRANSITIONS)
{
	std::uint16_t data = 42240;
	auto addr = 0x100;
	auto bs = read_and_track(addr, data);

	ASSERT_EQ(addr, bs.address);
	ASSERT_EQ(data, bs.data);
	ASSERT_TRUE(bs.uds_is_set);
	ASSERT_TRUE(bs.lds_is_set);
}

TEST(M68K_BUS_MANAGER, WRITE_WORD_BUS_TRANSITIONS)
{
	std::uint16_t data = 42241;
	auto addr = 0x100;
	auto bs = write_and_track(addr, data);

	ASSERT_EQ(addr, bs.address);
	ASSERT_EQ(data, bs.data);
	ASSERT_TRUE(bs.uds_is_set);
	ASSERT_TRUE(bs.lds_is_set);
}

TEST(M68K_BUS_MANAGER, RELEASE_BUS_WITHOUT_REQUEST)
{
	test::test_cpu cpu;
	auto& busm = cpu.bus_manager();

	ASSERT_THROW(busm.release_bus(), std::runtime_error);
}

TEST(M68K_BUS_MANAGER, RELEASE_NOT_GRANTED_BUS)
{
	test::test_cpu cpu;
	auto& busm = cpu.bus_manager();

	busm.request_bus();

	ASSERT_THROW(busm.release_bus(), std::runtime_error);
}

TEST(M68K_BUS_MANAGER, REQUEST_BUS)
{
	test::test_cpu cpu;
	auto& busm = cpu.bus_manager();

	busm.request_bus();
	auto cycles = cpu.cycle_until([&]() { return busm.bus_granted(); });

	ASSERT_TRUE(busm.bus_granted());
	ASSERT_LE(cycles, 15);
}

TEST(M68K_BUS_MANAGER, REQUEST_BUS_WHEN_ALREADY_GRANTED)
{
	test::test_cpu cpu;
	auto& busm = cpu.bus_manager();

	busm.request_bus();
	auto cycles = cpu.cycle_until([&]() { return busm.bus_granted(); });

	ASSERT_THROW(busm.request_bus(), std::runtime_error);
}

TEST(M68K_BUS_MANAGER, REQUEST_BUS_TWICE)
{
	test::test_cpu cpu;
	auto& busm = cpu.bus_manager();

	busm.request_bus();
	ASSERT_THROW(busm.request_bus(), std::runtime_error);
}
