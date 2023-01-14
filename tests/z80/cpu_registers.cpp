#include "z80/cpu_registers.hpp"

#include "string_utils.hpp"

#include <gtest/gtest.h>


TEST(Z80RegisterSet, RegisterPairing)
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


TEST(Z80CPURegisters, InitialValues)
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


TEST(Z80CPURegisters, NumberRepresination)
{
	auto regs = genesis::z80::cpu_registers();

	regs.main_set.A = -5;

	ASSERT_EQ(su::bin_str(regs.main_set.A), "11111011");
}

TEST(Z80CPURegisters, Flags)
{
	auto regs = genesis::z80::cpu_registers();

	auto check_flags = [&regs](std::string_view expected) {
		ASSERT_EQ(expected, su::bin_str(regs.main_set.F));
		regs.main_set.F = 0;
	};

	auto& flags = regs.main_set.flags;

	/* check individual flag */
	flags.S = 1;
	check_flags("10000000");

	flags.Z = 1;
	check_flags("01000000");

	flags.H = 1;
	check_flags("00010000");

	flags.PV = 1;
	check_flags("00000100");

	flags.N = 1;
	check_flags("00000010");

	flags.C = 1;
	check_flags("00000001");

	/* check flag pairs */
	flags.S = flags.C = 1;
	check_flags("10000001");

	flags.S = flags.N = 1;
	check_flags("10000010");

	flags.Z = flags.H = 1;
	check_flags("01010000");

	flags.PV = flags.C = 1;
	check_flags("00000101");

	/* check all flags */
	flags.S = flags.Z = flags.H = flags.PV = flags.N = flags.C = 1;
	check_flags("11010111");
}
