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
			{0x83, regs.main_set.E}, {0x84, regs.main_set.H}, {0x85, regs.main_set.L}};

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

	/* ADD A, n */
	{
		regs.main_set.A = 0x13;
		execute(cpu, {0xC6, 0x15});
		ASSERT_EQ(regs.main_set.A, 0x13 + 0x15);
	}

	auto test_index_reg = [&](genesis::z80::opcode opcode, std::uint16_t& idx_reg)
	{
		regs.main_set.A = 0x13;
		idx_reg = reserved + 0x22;
		std::uint8_t offset = 0x15;

		mem->write<std::uint8_t>(idx_reg + offset, 0x17);

		execute(cpu, {opcode, 0x86, offset});
		ASSERT_EQ(regs.main_set.A, 0x13 + 0x17);
	};

	/* ADD A, (IX + d) */
	test_index_reg(0xDD, regs.IX);

	/* ADD A, (IY + d) */
	test_index_reg(0xFD, regs.IY);
}


TEST(Z80ArithmeticLogicUnit, ADC)
{
	auto mem = std::make_shared<genesis::z80::memory>();
	auto cpu = genesis::z80::cpu(mem);
	auto& regs = cpu.registers();

	/* ADC r */
	{
		using test_pair = std::pair<genesis::z80::opcode, std::int8_t&>;

		std::initializer_list<test_pair> test_suites = {
			{0x8F, regs.main_set.A}, {0x88, regs.main_set.B}, {0x89, regs.main_set.C}, {0x8A, regs.main_set.D},
			{0x8B, regs.main_set.E}, {0x8C, regs.main_set.H}, {0x8D, regs.main_set.L}};

		for(int c = 0; c <= 1; ++c)
		{
			for (auto& s : test_suites)
			{
				auto& [op, r] = s;
				regs.main_set.A = 0x17;
				r = 0x12;

				auto expected = regs.main_set.A + r + c;
				regs.main_set.flags.C = c;

				execute(cpu, {op});
				ASSERT_EQ(regs.main_set.A, expected);
			}
		}
	}
}
