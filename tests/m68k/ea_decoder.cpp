#include <gtest/gtest.h>
#include <memory>

#include "m68k/impl/ea_decoder.hpp"

#define setup_test() \
	auto cpu = m68k::make_cpu(); \
	m68k::ea_decoder dec(cpu.bus_manager(), cpu.registers(), cpu.prefetch_queue())

using namespace genesis;


std::uint32_t decode(m68k::cpu& cpu, m68k::ea_decoder& dec, std::uint8_t ea, std::uint8_t size,
	std::initializer_list<std::uint8_t> mem_data)
{
	if(mem_data.size() == 0)
		throw std::runtime_error("decode error: mem data supposed to have at least 1 element");

	auto& regs = cpu.registers();
	regs.PC = 0x101;

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
	std::uint32_t cycles = 0;
	while(!dec.ready())
	{
		dec.cycle();
	}

	return cycles;
}

const std::uint8_t num_regs = 8;

TEST(M68K_EA_DECODER, MODE_000)
{
	setup_test();

	auto& regs = cpu.registers();
	const std::uint8_t data_mode = 0;
	for(std::size_t i = 0; i < num_regs; ++i)
	{
		std::uint8_t ea = data_mode + i;
		auto cycles = decode(cpu, dec, ea, 1, { 0x0 });
		auto op = dec.result();

		// immidiate decoding is cycless
		ASSERT_EQ(0, cycles);

		ASSERT_TRUE(op.has_data_reg());
		ASSERT_FALSE(op.has_addr_reg());
		ASSERT_FALSE(op.has_pointer());
		ASSERT_EQ(std::addressof(regs.D(i)), std::addressof(op.data_reg()));
	}
}

TEST(M68K_EA_DECODER, MODE_001)
{
	setup_test();

	auto& regs = cpu.registers();
	const std::uint8_t addr_mode = 1 << 3;
	for(std::size_t i = 0; i < num_regs; ++i)
	{
		std::uint8_t ea = addr_mode + i;
		auto cycles = decode(cpu, dec, ea, 1, { 0x0 });
		auto op = dec.result();

		// immidiate decoding is cycless
		ASSERT_EQ(0, cycles);

		ASSERT_TRUE(op.has_addr_reg());
		ASSERT_FALSE(op.has_data_reg());
		ASSERT_FALSE(op.has_pointer());
		ASSERT_EQ(std::addressof(regs.A(i)), std::addressof(op.addr_reg()));
	}
}
