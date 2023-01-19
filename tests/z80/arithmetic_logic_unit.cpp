#include "z80/cpu.h"

#include "string_utils.hpp"

#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <initializer_list>

using namespace genesis;

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

z80::cpu make_cpu()
{
	return z80::cpu(std::make_shared<genesis::z80::memory>());
}

using bin_op_fn = std::function<std::int8_t(std::int8_t, std::int8_t, const genesis::z80::cpu_registers& regs)>;

using reg_opcode_pair = std::pair<genesis::z80::opcode, std::int8_t&>;
void test_r(genesis::z80::cpu& cpu, std::initializer_list<reg_opcode_pair> reg_opcode_sets, bin_op_fn bin_op)
{
	auto& regs = cpu.registers();
	for(auto& [opcode, reg] : reg_opcode_sets)
	{
		for(int c = 0; c <= 1; ++c)
		{
			/* setup registers for test */
			regs.main_set.A = 0x13;
			regs.main_set.flags.C = c;
			reg = 0x15;

			auto expected = bin_op(regs.main_set.A, reg, regs);
			execute(cpu, {opcode});
			auto actual = regs.main_set.A;

			ASSERT_EQ(expected, actual) << "failed to execute " << su::hex_str(opcode);
		}
	}
}

void test_n(genesis::z80::cpu& cpu, genesis::z80::opcode opcode, bin_op_fn bin_op)
{
	const std::uint8_t n = 0x15;
	auto& regs = cpu.registers();
	for(int c = 0; c <= 1; ++c)
	{
		/* setup registers for test */
		regs.main_set.A = 0x13;
		regs.main_set.flags.C = c;

		auto expected = bin_op(regs.main_set.A, n, regs);
		execute(cpu, {opcode, n});
		auto actual = regs.main_set.A;

		ASSERT_EQ(expected, actual) << "failed to execute " << su::hex_str(opcode) << " " << su::hex_str(n);
	}
}

void test_hl(z80::cpu& cpu, z80::opcode opcode, bin_op_fn bin_op)
{
	const std::int8_t b = 0xb;
	const z80::memory::address addr = reserved + 0xef;

	cpu.memory().write(addr, b);

	auto& regs = cpu.registers();
	for(int c = 0; c <= 1; ++c)
	{
		/* setup registers for test */
		regs.main_set.A = 0xA;
		regs.main_set.HL = addr;
		regs.main_set.flags.C = c;

		auto expected = bin_op(regs.main_set.A, b, regs);
		execute(cpu, {opcode});
		auto actual = regs.main_set.A;

		ASSERT_EQ(expected, actual) << "failed to execute " << su::hex_str(opcode);
	}
}

void test_idx(z80::cpu& cpu, z80::opcode opcode, bin_op_fn bin_op)
{
	const std::uint8_t d = 0x14;
	const z80::memory::address base = reserved + 0x20;
	const std::uint8_t b = 0x17;

	cpu.memory().write(base + d, b);

	auto& regs = cpu.registers();
	for(z80::opcode op : {0xDD, 0xFD})
	{
		for(int c = 0; c <= 1; ++c)
		{
			/* setup registers for test */
			auto& reg = op == 0xDD ? regs.IX : regs.IY;
			reg = base;
			regs.main_set.A = 0x21;
			regs.main_set.flags.C = c;

			auto expected = bin_op(regs.main_set.A, b, regs);
			execute(cpu, {op, opcode, d});
			auto actual = regs.main_set.A;

			ASSERT_EQ(expected, actual) << "failed to execute "
				<< su::hex_str(op) << ' '
				<< su::hex_str(opcode) << ' '
				<< su::hex_str(d);
		}
	}
}

TEST(Z80ArithmeticLogicUnit, ADD)
{
	auto cpu = make_cpu();

	auto add = [](std::int8_t a, std::int8_t b, const genesis::z80::cpu_registers&) -> std::int8_t
	{
		return a + b;
	};

	/* ADD r */
	{
		auto& regs = cpu.registers();
		std::initializer_list<reg_opcode_pair> op_reg_pairs = {
			{0x87, regs.main_set.A}, {0x80, regs.main_set.B}, {0x81, regs.main_set.C}, {0x82, regs.main_set.D},
			{0x83, regs.main_set.E}, {0x84, regs.main_set.H}, {0x85, regs.main_set.L}};

		test_r(cpu, op_reg_pairs, add);
	}

	/* ADD A, n */
	test_n(cpu, 0xC6, add);

	/* ADD HL */
	test_hl(cpu, 0x86, add);

	/* ADD A, (IX + d) */
	/* ADD A, (IY + d) */
	test_idx(cpu, 0x86, add);
}


TEST(Z80ArithmeticLogicUnit, ADC)
{
	auto cpu = make_cpu();

	auto adc = [](std::int8_t a, std::int8_t b, const genesis::z80::cpu_registers& regs) -> std::int8_t
	{
		return a + b + regs.main_set.flags.C;
	};

	/* ADC r */
	{
		auto& regs = cpu.registers();
		std::initializer_list<reg_opcode_pair> op_reg_pairs = {
			{0x8F, regs.main_set.A}, {0x88, regs.main_set.B}, {0x89, regs.main_set.C}, {0x8A, regs.main_set.D},
			{0x8B, regs.main_set.E}, {0x8C, regs.main_set.H}, {0x8D, regs.main_set.L}};

		test_r(cpu, op_reg_pairs, adc);
	}

	/* ADC A, n */
	test_n(cpu, 0xCE, adc);

	/* ADC HL */
	test_hl(cpu, 0x8E, adc);

	/* ADC A, (IX + d) */
	/* ADC A, (IY + d) */
	test_idx(cpu, 0x8E, adc);
}

TEST(Z80ArithmeticLogicUnit, SUB)
{
	auto cpu = make_cpu();

	auto sub = [](std::int8_t a, std::int8_t b, const genesis::z80::cpu_registers&) -> std::int8_t
	{
		return a - b;
	};

	/* ADC r */
	{
		auto& regs = cpu.registers();
		std::initializer_list<reg_opcode_pair> op_reg_pairs = {
			{0x97, regs.main_set.A}, {0x90, regs.main_set.B}, {0x91, regs.main_set.C}, {0x92, regs.main_set.D},
			{0x93, regs.main_set.E}, {0x94, regs.main_set.H}, {0x95, regs.main_set.L}};

		test_r(cpu, op_reg_pairs, sub);
	}

	/* ADC A, n */
	test_n(cpu, 0xD6, sub);

	/* ADC HL */
	test_hl(cpu, 0x96, sub);

	/* ADC A, (IX + d) */
	/* ADC A, (IY + d) */
	test_idx(cpu, 0x96, sub);
}

TEST(Z80ArithmeticLogicUnit, SBC)
{
	auto cpu = make_cpu();

	auto sbc = [](std::int8_t a, std::int8_t b, const genesis::z80::cpu_registers& regs) -> std::int8_t
	{
		return a - b - regs.main_set.flags.C;
	};

	/* ADC r */
	{
		auto& regs = cpu.registers();
		std::initializer_list<reg_opcode_pair> op_reg_pairs = {
			{0x9F, regs.main_set.A}, {0x98, regs.main_set.B}, {0x99, regs.main_set.C}, {0x9A, regs.main_set.D},
			{0x9B, regs.main_set.E}, {0x9C, regs.main_set.H}, {0x9D, regs.main_set.L}};

		test_r(cpu, op_reg_pairs, sbc);
	}

	/* ADC A, n */
	test_n(cpu, 0xDE, sbc);

	/* ADC HL */
	test_hl(cpu, 0x9E, sbc);

	/* ADC A, (IX + d) */
	/* ADC A, (IY + d) */
	test_idx(cpu, 0x9E, sbc);
}

TEST(Z80ArithmeticLogicUnit, AND_8)
{
	auto cpu = make_cpu();

	auto add_8 = [](std::int8_t a, std::int8_t b, const genesis::z80::cpu_registers&) -> std::int8_t
	{
		return (std::uint8_t)a & (std::uint8_t)b;
	};

	/* ADC r */
	{
		auto& regs = cpu.registers();
		std::initializer_list<reg_opcode_pair> op_reg_pairs = {
			{0xA7, regs.main_set.A}, {0xA0, regs.main_set.B}, {0xA1, regs.main_set.C}, {0xA2, regs.main_set.D},
			{0xA3, regs.main_set.E}, {0xA4, regs.main_set.H}, {0xA5, regs.main_set.L}};

		test_r(cpu, op_reg_pairs, add_8);
	}

	/* ADC A, n */
	test_n(cpu, 0xE6, add_8);

	/* ADC HL */
	test_hl(cpu, 0xA6, add_8);

	/* ADC A, (IX + d) */
	/* ADC A, (IY + d) */
	test_idx(cpu, 0xA6, add_8);
}

TEST(Z80ArithmeticLogicUnit, OR_8)
{
	auto cpu = make_cpu();

	auto or_8 = [](std::int8_t a, std::int8_t b, const genesis::z80::cpu_registers&) -> std::int8_t
	{
		return (std::uint8_t)a | (std::uint8_t)b;
	};

	/* OR r */
	{
		auto& regs = cpu.registers();
		std::initializer_list<reg_opcode_pair> op_reg_pairs = {
			{0xB7, regs.main_set.A}, {0xB0, regs.main_set.B}, {0xB1, regs.main_set.C}, {0xB2, regs.main_set.D},
			{0xB3, regs.main_set.E}, {0xB4, regs.main_set.H}, {0xB5, regs.main_set.L}};

		test_r(cpu, op_reg_pairs, or_8);
	}

	/* OR A, n */
	test_n(cpu, 0xF6, or_8);

	/* OR HL */
	test_hl(cpu, 0xB6, or_8);

	/* OR A, (IX + d) */
	/* OR A, (IY + d) */
	test_idx(cpu, 0xB6, or_8);
}

TEST(Z80ArithmeticLogicUnit, XOR)
{
	auto cpu = make_cpu();

	auto xor_8 = [](std::int8_t a, std::int8_t b, const genesis::z80::cpu_registers&) -> std::int8_t
	{
		return (std::uint8_t)a ^ (std::uint8_t)b;
	};

	/* XOR r */
	{
		auto& regs = cpu.registers();
		std::initializer_list<reg_opcode_pair> op_reg_pairs = {
			{0xAF, regs.main_set.A}, {0xA8, regs.main_set.B}, {0xA9, regs.main_set.C}, {0xAA, regs.main_set.D},
			{0xAB, regs.main_set.E}, {0xAC, regs.main_set.H}, {0xAD, regs.main_set.L}};

		test_r(cpu, op_reg_pairs, xor_8);
	}

	/* OR A, n */
	test_n(cpu, 0xEE, xor_8);

	/* OR HL */
	test_hl(cpu, 0xAE, xor_8);

	/* OR A, (IX + d) */
	/* OR A, (IY + d) */
	test_idx(cpu, 0xAE, xor_8);
}
