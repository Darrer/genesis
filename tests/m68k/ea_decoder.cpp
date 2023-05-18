#include <gtest/gtest.h>
#include <memory>

#include "m68k/impl/ea_decoder.hpp"
#include "m68k/impl/size_type.h"
#include "test_cpu.hpp"


#define setup_test() \
	genesis::test::test_cpu cpu; \
	m68k::ea_decoder dec(cpu.registers(), cpu.bus_scheduler())

using namespace genesis::m68k;
using namespace genesis;


void decode(test::test_cpu& cpu, m68k::ea_decoder& dec, std::uint8_t ea, m68k::size_type size,
	std::initializer_list<std::uint8_t> mem_data)
{
	if(mem_data.size() == 0)
		throw std::runtime_error("decode error: mem data supposed to have at least 1 element");

	auto& regs = cpu.registers();
	regs.PC = 0x100;

	// setup mem
	regs.IRC = *mem_data.begin();
	std::uint32_t offset = 0;
	for(auto it = std::next(mem_data.begin()); it != mem_data.end(); ++it)
	{
		cpu.memory().write(regs.PC + offset, *it);
		offset += sizeof(*it);
	}

	// start decoding
	dec.schedule_decoding(ea, size);
	while(!dec.ready())
	{
		cpu.bus_scheduler().cycle();
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
		decode(cpu, dec, ea, m68k::size_type::BYTE, { 0x0 });
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
		decode(cpu, dec, ea, m68k::size_type::BYTE, { 0x0 });
		auto op = dec.result();

		ASSERT_TRUE(op.is_addr_reg());
		ASSERT_FALSE(op.is_data_reg());
		ASSERT_FALSE(op.is_pointer());
		ASSERT_EQ(std::addressof(regs.A(i)), std::addressof(op.addr_reg()));
	}
}

void check_timings(std::uint8_t ea, m68k::size_type size,
	std::uint8_t expected_total_cycles, std::uint8_t expected_bus_read_cycles)
{
	setup_test();

	auto& bus = cpu.bus();

	std::uint32_t base = 0x100;

	cpu.registers().A0.LW = base;
	cpu.registers().PC = base;
	cpu.memory().write(base, std::uint32_t(base));
	dec.schedule_decoding(ea, size);

	std::uint32_t cycles = 0;
	std::uint32_t read_cycles = 0;

	bool in_read_cycle = false;
	while (!dec.ready() || !cpu.bus_scheduler().is_idle())
	{
		cpu.bus_scheduler().cycle();
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
	for(auto sz : {size_type::BYTE, size_type::WORD, size_type::LONG})
		check_timings(0b000, sz, 0, 0);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_001)
{
	for(auto sz : {size_type::BYTE, size_type::WORD, size_type::LONG})
		check_timings(0b001 << 3, sz, 0, 0);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_010)
{
	check_timings(0b010 << 3, size_type::BYTE, 4, 1);
	check_timings(0b010 << 3, size_type::WORD, 4, 1);
	check_timings(0b010 << 3, size_type::LONG, 8, 2);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_011)
{
	check_timings(0b011 << 3, size_type::BYTE, 4, 1);
	check_timings(0b011 << 3, size_type::WORD, 4, 1);
	check_timings(0b011 << 3, size_type::LONG, 8, 2);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_100)
{
	check_timings(0b100 << 3, size_type::BYTE, 6, 1);
	check_timings(0b100 << 3, size_type::WORD, 6, 1);
	check_timings(0b100 << 3, size_type::LONG, 10, 2);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_101)
{
	check_timings(0b101 << 3, size_type::BYTE, 8, 2);
	check_timings(0b101 << 3, size_type::WORD, 8, 2);
	check_timings(0b101 << 3, size_type::LONG, 12, 3);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_110)
{
	check_timings(0b110 << 3, size_type::BYTE, 10, 2);
	check_timings(0b110 << 3, size_type::WORD, 10, 2);
	check_timings(0b110 << 3, size_type::LONG, 14, 3);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_111_000)
{
	check_timings(0b111000, size_type::BYTE, 8, 2);
	check_timings(0b111000, size_type::WORD, 8, 2);
	check_timings(0b111000, size_type::LONG, 12, 3);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_111_001)
{
	check_timings(0b111001, size_type::BYTE, 12, 3);
	check_timings(0b111001, size_type::WORD, 12, 3);
	check_timings(0b111001, size_type::LONG, 16, 4);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_111_010)
{
	check_timings(0b111010, size_type::BYTE, 8, 2);
	check_timings(0b111010, size_type::WORD, 8, 2);
	check_timings(0b111010, size_type::LONG, 12, 3);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_111_011)
{
	check_timings(0b111011, size_type::BYTE, 10, 2);
	check_timings(0b111011, size_type::WORD, 10, 2);
	check_timings(0b111011, size_type::LONG, 14, 3);
}

TEST(M68K_EA_DECODER_TIMINGS, MODE_111_100)
{
	check_timings(0b111100, size_type::BYTE, 4, 1);
	check_timings(0b111100, size_type::WORD, 4, 1);
	check_timings(0b111100, size_type::LONG, 8, 2);
}
