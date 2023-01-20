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

void reset_reg(z80::cpu_registers& regs)
{
	regs.I = regs.R = 0x0;
	regs.IX = regs.IY = regs.SP = regs.PC = 0x0;

	regs.main_set.AF = regs.main_set.BC = regs.main_set.DE = regs.main_set.HL = 0x0;
	regs.alt_set.AF = regs.alt_set.BC = regs.alt_set.DE = regs.alt_set.HL = 0x0;
}

z80::cpu make_cpu()
{
	return z80::cpu(std::make_shared<genesis::z80::memory>());
}

using bin_op_fn = std::function<std::int8_t(std::int8_t, std::int8_t, const genesis::z80::cpu_registers& regs)>;
using un_op_fn = std::function<std::int8_t(std::int8_t, const genesis::z80::cpu_registers& regs)>;

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

void test_r(genesis::z80::cpu& cpu, std::initializer_list<reg_opcode_pair> reg_opcode_sets, un_op_fn un_op)
{
	auto& regs = cpu.registers();
	for(auto& [opcode, reg] : reg_opcode_sets)
	{
		for(int c = 0; c <= 1; ++c)
		{
			/* setup registers for test */
			regs.main_set.flags.C = c;
			reg = 0x15;

			auto expected = un_op(reg, regs);
			execute(cpu, {opcode});
			auto actual = reg;

			ASSERT_EQ(expected, actual) << "failed to execute " << su::hex_str(opcode);
		}
	}
}

using reg_opcode_bin_pair = std::pair<genesis::z80::opcode, std::pair<std::int8_t&, std::int8_t&>>;
void test_binary_r(genesis::z80::cpu& cpu, std::initializer_list<reg_opcode_bin_pair> reg_opcode_bin_sets, bin_op_fn bin_op)
{
	auto& regs = cpu.registers();
	for(auto& [opcode, reg] : reg_opcode_bin_sets)
	{
		for(int c = 0; c <= 1; ++c)
		{
			reset_reg(regs);

			/* setup registers for test */
			regs.main_set.flags.C = c;
			reg.first = 0x14;
			reg.second = 0x15;

			auto expected = bin_op(reg.first, reg.second, regs);
			execute(cpu, {opcode});
			auto actual = reg.first;

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

/* write block */

void test_hl_write(z80::cpu& cpu, z80::opcode opcode, un_op_fn un_op)
{
	const std::int8_t b = 0xb;
	const z80::memory::address addr = reserved + 0xef;

	auto& regs = cpu.registers();
	for(int c = 0; c <= 1; ++c)
	{
		cpu.memory().write(addr, b);

		/* setup registers for test */
		regs.main_set.HL = addr;
		regs.main_set.flags.C = c;

		auto expected = un_op(b, regs);
		execute(cpu, {opcode});
		auto actual = cpu.memory().read<std::int8_t>(addr);

		ASSERT_EQ(expected, actual) << "failed to execute " << su::hex_str(opcode);
	}
}

void test_idx_write(z80::cpu& cpu, z80::opcode opcode, un_op_fn un_op)
{
	const std::uint8_t d = 0x14;
	const z80::memory::address base = reserved + 0x20;
	const z80::memory::address addr = base + d;
	const std::uint8_t b = 0x17;

	auto& regs = cpu.registers();
	for(z80::opcode op : {0xDD, 0xFD})
	{
		for(int c = 0; c <= 1; ++c)
		{
			cpu.memory().write(addr, b);

			/* setup registers for test */
			auto& reg = op == 0xDD ? regs.IX : regs.IY;
			reg = base;
			regs.main_set.flags.C = c;

			auto expected = un_op(b, regs);
			execute(cpu, {op, opcode, d});
			auto actual = cpu.memory().read<std::int8_t>(addr);

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

TEST(Z80ArithmeticLogicUnit, CP)
{
	auto cpu = make_cpu();

	auto cp = [](std::int8_t a, std::int8_t, const genesis::z80::cpu_registers&) -> std::int8_t
	{
		return a;
	};

	/* CP r */
	{
		auto& regs = cpu.registers();
		std::initializer_list<reg_opcode_pair> op_reg_pairs = {
			{0xBF, regs.main_set.A}, {0xB8, regs.main_set.B}, {0xB9, regs.main_set.C}, {0xBA, regs.main_set.D},
			{0xBB, regs.main_set.E}, {0xBC, regs.main_set.H}, {0xBD, regs.main_set.L}};

		test_r(cpu, op_reg_pairs, cp);
	}

	/* CP A, n */
	test_n(cpu, 0xFE, cp);

	/* CP HL */
	test_hl(cpu, 0xBE, cp);

	/* CP A, (IX + d) */
	/* CP A, (IY + d) */
	test_idx(cpu, 0xBE, cp);
}

TEST(Z80ArithmeticLogicUnit, INC)
{
	auto cpu = make_cpu();

	auto inc = [](std::int8_t r, const genesis::z80::cpu_registers&) -> std::int8_t
	{
		return r + 1;
	};

	/* INC r */
	{
		auto& regs = cpu.registers();
		std::initializer_list<reg_opcode_pair> op_reg_pairs = {
			{0x3C, regs.main_set.A}, {0x04, regs.main_set.B}, {0x0C, regs.main_set.C}, {0x14, regs.main_set.D},
			{0x1C, regs.main_set.E}, {0x24, regs.main_set.H}, {0x2C, regs.main_set.L}};

		test_r(cpu, op_reg_pairs, inc);
	}

	/* INC HL */
	test_hl_write(cpu, 0x34, inc);

	/* INC A, (IX + d) */
	/* INC A, (IY + d) */
	test_idx_write(cpu, 0x34, inc);
}

TEST(Z80ArithmeticLogicUnit, DEC)
{
	auto cpu = make_cpu();

	auto dec = [](std::int8_t r, const genesis::z80::cpu_registers&) -> std::int8_t
	{
		return r - 1;
	};

	/* DEC r */
	{
		auto& regs = cpu.registers();
		std::initializer_list<reg_opcode_pair> op_reg_pairs = {
			{0x3D, regs.main_set.A}, {0x05, regs.main_set.B}, {0x0D, regs.main_set.C}, {0x15, regs.main_set.D},
			{0x1D, regs.main_set.E}, {0x25, regs.main_set.H}, {0x2D, regs.main_set.L}};

		test_r(cpu, op_reg_pairs, dec);
	}

	/* DEC HL */
	test_hl_write(cpu, 0x35, dec);

	/* DEC A, (IX + d) */
	/* DEC A, (IY + d) */
	test_idx_write(cpu, 0x35, dec);
}

TEST(Z80ArithmeticLogicUnit, LD)
{
	auto cpu = make_cpu();

	auto ld = [](std::int8_t, std::int8_t r, const genesis::z80::cpu_registers&) -> std::int8_t
	{
		return r;
	};

	/* LD r */
	{
		auto& r = cpu.registers().main_set;
		std::initializer_list<reg_opcode_bin_pair> op_reg_pairs = {
			{0x7F, {r.A, r.A}}, {0x78, {r.A, r.B}}, {0x79, {r.A, r.C}}, {0x7A, {r.A, r.D}},
			{0x7B, {r.A, r.E}}, {0x7C, {r.A, r.H}}, {0x7D, {r.A, r.L}},

			{0x47, {r.B, r.A}}, {0x40, {r.B, r.B}}, {0x41, {r.B, r.C}}, {0x42, {r.B, r.D}},
			{0x43, {r.B, r.E}}, {0x44, {r.B, r.H}}, {0x45, {r.B, r.L}},

			{0x4F, {r.C, r.A}}, {0x48, {r.C, r.B}}, {0x49, {r.C, r.C}}, {0x4A, {r.C, r.D}},
			{0x4B, {r.C, r.E}}, {0x4C, {r.C, r.H}}, {0x4D, {r.C, r.L}},

			{0x57, {r.D, r.A}}, {0x50, {r.D, r.B}}, {0x51, {r.D, r.C}}, {0x52, {r.D, r.D}},
			{0x53, {r.D, r.E}}, {0x54, {r.D, r.H}}, {0x55, {r.D, r.L}},

			{0x5F, {r.E, r.A}}, {0x58, {r.E, r.B}}, {0x59, {r.E, r.C}}, {0x5A, {r.E, r.D}},
			{0x5B, {r.E, r.E}}, {0x5C, {r.E, r.H}}, {0x5D, {r.E, r.L}},

			{0x67, {r.H, r.A}}, {0x60, {r.H, r.B}}, {0x61, {r.H, r.C}}, {0x62, {r.H, r.D}},
			{0x63, {r.H, r.E}}, {0x64, {r.H, r.H}}, {0x65, {r.H, r.L}},

			{0x6F, {r.L, r.A}}, {0x68, {r.L, r.B}}, {0x69, {r.L, r.C}}, {0x6A, {r.L, r.D}},
			{0x6B, {r.L, r.E}}, {0x6C, {r.L, r.H}}, {0x6D, {r.L, r.L}},
		};

		test_binary_r(cpu, op_reg_pairs, ld);
	}

	/* DEC HL */
	// test_hl_write(cpu, 0x35, dec);

	// /* DEC A, (IX + d) */
	// /* DEC A, (IY + d) */
	// test_idx_write(cpu, 0x35, dec);
}
