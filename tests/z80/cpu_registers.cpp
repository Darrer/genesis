#include "z80/cpu_registers.hpp"

#include <gtest/gtest.h>


TEST(RegisterSet, RegisterPairing)
{
	auto check_regs = [](auto& first, auto& second, auto& pair) {
		first = 0x19;
		ASSERT_EQ(first, 0x19);

		second = 0x28;
		ASSERT_EQ(second, 0x28);

		ASSERT_EQ(pair, 0x1928);

		// --

		pair = 0x1234;
		ASSERT_EQ(pair, 0x1234);

		ASSERT_EQ(first, 0x12);
		ASSERT_EQ(second, 0x34);
	};

	auto regs = genesis::z80::register_set();

	check_regs(regs.A, regs.F, regs.AF);
	check_regs(regs.B, regs.C, regs.BC);
	check_regs(regs.D, regs.E, regs.DE);
	check_regs(regs.H, regs.L, regs.HL);
}


TEST(CPURegisters, InitialValues)
{
	auto assert_all_zeros = [](const auto& arr) {
		for (const auto& i : arr)
			ASSERT_EQ(i, 0x0);
	};

	auto check_reg_set = [&assert_all_zeros](const auto& regs) {
		auto ind_regs = {regs.A, regs.B, regs.C, regs.D, regs.E, regs.F, regs.H, regs.L};
		auto paired_regs = {regs.AF, regs.BC, regs.DE, regs.HL};

		assert_all_zeros(ind_regs);
		assert_all_zeros(paired_regs);
	};

	auto regs = genesis::z80::cpu_registers();

	check_reg_set(regs.main_set);
	check_reg_set(regs.alt_set);

	auto sp1 = {regs.I, regs.R};
	auto sp2 = {regs.IX, regs.IY, regs.PC, regs.SP};

	assert_all_zeros(sp1);
	assert_all_zeros(sp2);
}
