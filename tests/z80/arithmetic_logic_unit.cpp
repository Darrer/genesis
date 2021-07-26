#include "z80/cpu.h"

#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <initializer_list>

const std::uint16_t reserved = 0x100;

using verify_cb = std::function<bool()>;


void execute(genesis::z80::cpu& cpu, std::initializer_list<std::uint8_t> opcode, verify_cb verify_fn = nullptr)
{
	if (opcode.size() >= reserved)
	{
		throw std::runtime_error("Internal error, only 256 bytes reserved for opcode!");
	}

	// write opcode into memory
	auto& mem = cpu.memory();
	std::uint16_t offset = 0x0;
	for (auto i : opcode)
		mem.write(offset++, i);

	// set up PC
	cpu.registers().PC = 0x0;

	cpu.execute_one();

	// NOTE: won't work for jump instructions
	ASSERT_EQ(cpu.registers().PC, 0x0 + opcode.size());

	if (verify_fn)
		verify_fn();
}


TEST(Z80ArithmeticLogicUnit, ADD)
{
	auto mem = std::make_shared<genesis::z80::memory>();
	auto cpu = genesis::z80::cpu(mem);
	auto& regs = cpu.registers();

	/* ADD HL */
	{
		regs.main_set.A = 0x13;
		regs.main_set.HL = reserved + 0x20;

		mem->write<std::uint8_t>(regs.main_set.HL, 0x31);

		execute(cpu, {0x86});
		ASSERT_EQ(regs.main_set.A, 0x13 + 0x31);
	}

	/* ADD r */
	{
		using test_pair = std::pair<genesis::z80::opcode, std::int8_t&>;

		std::initializer_list<test_pair> test_suites = {
			{0x87, regs.main_set.A}, {0x80, regs.main_set.B}, {0x81, regs.main_set.C}, {0x82, regs.main_set.D},
			{0x83, regs.main_set.E}, {0x84, regs.main_set.F}, {0x85, regs.main_set.L}};

		for (auto& s : test_suites)
		{
			auto& [op, r] = s;
			regs.main_set.A = 0x17;
			r = 0x12;

			auto expected = regs.main_set.A + r;

			execute(cpu, {op});
			ASSERT_EQ(regs.main_set.A, expected);
		}
	}
}
