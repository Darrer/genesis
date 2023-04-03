#include <gtest/gtest.h>
#include <memory>

#include "m68k/impl/ea_decoder.hpp"
#include "test_cpu.hpp"


#define setup_test() \
	genesis::test::test_cpu cpu; \
	m68k::ea_decoder dec(cpu.bus_manager(), cpu.registers(), cpu.prefetch_queue(), cpu.bus_scheduler())

using namespace genesis;


void decode(test::test_cpu& cpu, m68k::ea_decoder& dec, std::uint8_t ea, std::uint8_t size,
	std::initializer_list<std::uint8_t> mem_data)
{
	if(mem_data.size() == 0)
		throw std::runtime_error("decode error: mem data supposed to have at least 1 element");

	auto& regs = cpu.registers();
	regs.PC = 0x100;

	// setup mem
	cpu.prefetch_queue().IRC = *mem_data.begin();
	std::uint32_t offset = 0;
	for(auto it = std::next(mem_data.begin()); it != mem_data.end(); ++it)
	{
		cpu.memory().write(regs.PC + offset, *it);
		offset += sizeof(*it);
	}

	// start decoding
	dec.decode(ea, size);
	while(!dec.ready())
	{
		dec.cycle();
		cpu.bus_scheduler().cycle();
		cpu.prefetch_queue().cycle();
		cpu.bus_manager().cycle();
	}
}

const std::uint8_t num_regs = 8;

TEST(M68K_EA_DECODER, MODE_000)
{
	setup_test();

	auto& regs = cpu.registers();
	const std::uint8_t data_mode = 0;
	for(std::uint8_t i = 0; i < num_regs; ++i)
	{
		std::uint8_t ea = data_mode + i;
		decode(cpu, dec, ea, 1, { 0x0 });
		auto op = dec.result();

		ASSERT_TRUE(op.is_data_reg());
		ASSERT_FALSE(op.is_addr_reg());
		ASSERT_FALSE(op.is_pointer());
		ASSERT_EQ(std::addressof(regs.D(i)), std::addressof(op.data_reg()));
	}
}

TEST(M68K_EA_DECODER, MODE_001)
{
	setup_test();

	auto& regs = cpu.registers();
	const std::uint8_t addr_mode = 1 << 3;
	for(std::uint8_t i = 0; i < num_regs; ++i)
	{
		std::uint8_t ea = addr_mode + i;
		decode(cpu, dec, ea, 1, { 0x0 });
		auto op = dec.result();

		ASSERT_TRUE(op.is_addr_reg());
		ASSERT_FALSE(op.is_data_reg());
		ASSERT_FALSE(op.is_pointer());
		ASSERT_EQ(std::addressof(regs.A(i)), std::addressof(op.addr_reg()));
	}
}

void check_timings(std::uint8_t ea, std::uint8_t size,
	std::uint8_t expected_total_cycles, std::uint8_t expected_bus_read_cycles)
{
	setup_test();

	auto& bus = cpu.bus();

	std::uint32_t base = 0x100;

	cpu.registers().A0.LW = base;
	cpu.registers().PC = base;
	cpu.memory().write(base, std::uint32_t(base));
	dec.decode(ea, size);

	std::uint32_t cycles = 0;
	std::uint32_t read_cycles = 0;

	bool in_read_cycle = false;
	while (!dec.ready())
	{
		dec.cycle();
		cpu.bus_scheduler().cycle();
		cpu.prefetch_queue().cycle();
		cpu.bus_manager().cycle();

		++cycles;

		// check if in read cycle
		if(bus.is_set(m68k::bus::AS) && bus.is_set(m68k::bus::RW))
		{
			if(!in_read_cycle)
			{
				in_read_cycle = true;
				++read_cycles;
			}
		}
		else
		{
			in_read_cycle = false;
		}
	}

	ASSERT_EQ(expected_total_cycles, cycles);
	ASSERT_EQ(expected_bus_read_cycles, read_cycles);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_000)
{
	for(auto sz : {1, 2, 4})
		check_timings(0b000, sz, 0, 0);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_001)
{
	for(auto sz : {1, 2, 4})
		check_timings(0b001 << 3, sz, 0, 0);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_010)
{
	check_timings(0b010 << 3, 1, 4, 1);
	check_timings(0b010 << 3, 2, 4, 1);
	check_timings(0b010 << 3, 4, 8, 2);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_011)
{
	check_timings(0b011 << 3, 1, 4, 1);
	check_timings(0b011 << 3, 2, 4, 1);
	check_timings(0b011 << 3, 4, 8, 2);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_100)
{
	check_timings(0b100 << 3, 1, 6, 1);
	check_timings(0b100 << 3, 2, 6, 1);
	check_timings(0b100 << 3, 4, 10, 2);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_101)
{
	check_timings(0b101 << 3, 1, 8, 2);
	check_timings(0b101 << 3, 2, 8, 2);
	check_timings(0b101 << 3, 4, 12, 3);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_110)
{
	check_timings(0b110 << 3, 1, 10, 2);
	check_timings(0b110 << 3, 2, 10, 2);
	check_timings(0b110 << 3, 4, 14, 3);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_111_000)
{
	check_timings(0b111000, 1, 8, 2);
	check_timings(0b111000, 2, 8, 2);
	check_timings(0b111000, 4, 12, 3);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_111_001)
{
	check_timings(0b111001, 1, 12, 3);
	check_timings(0b111001, 2, 12, 3);
	check_timings(0b111001, 4, 16, 4);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_111_010)
{
	check_timings(0b111010, 1, 8, 2);
	check_timings(0b111010, 2, 8, 2);
	check_timings(0b111010, 4, 12, 3);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_111_011)
{
	check_timings(0b111011, 1, 10, 2);
	check_timings(0b111011, 2, 10, 2);
	check_timings(0b111011, 4, 14, 3);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_111_100)
{
	check_timings(0b111100, 1, 4, 1);
	check_timings(0b111100, 2, 4, 1);
	check_timings(0b111100, 4, 8, 2);
}
