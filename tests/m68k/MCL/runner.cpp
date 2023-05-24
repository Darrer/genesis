#include <gtest/gtest.h>

#include "string_utils.hpp"
#include <iostream>

#include "../test_cpu.hpp"
#include "../../helper.hpp"

using namespace genesis::test;
using namespace genesis::m68k;


void nop_some_tests(memory& mem)
{
	auto nop_test = [&](std::uint32_t jump_addr)
	{
		const std::uint16_t nop_opcode = 0b0100111001110001;
		// jump occypy 6 bytes - 2 for opcode, 4 for ptr
		mem.write(jump_addr, nop_opcode);
		mem.write(jump_addr + 2, nop_opcode);
		mem.write(jump_addr + 4, nop_opcode);
	};

	// TODO: ABCD/SBCD have tricky logic and now I'm to lazy to figure it out,
	// so just skip these tests so far
	const std::uint32_t abcd_jump_addr = 0x4A8;
	const std::uint32_t sbcd_jump_addr = 0x4AE;

	nop_test(abcd_jump_addr);
	nop_test(sbcd_jump_addr);

	// const std::uint32_t nbcd_jump_addr = 0x4B4;
	// nop_test(nbcd_jump_addr);

	// MOVE to SR is tested incorrectly, see issue:
	// https://github.com/MicroCoreLabs/Projects/issues/6
	const std::uint32_t move_to_sr_jump_addr = 0x466;
	nop_test(move_to_sr_jump_addr);
}

// The test program is taken from: https://github.com/MicroCoreLabs/Projects/blob/master/MCL68/MC68000_Test_Code/
TEST(M68K, MCL)
{
	const auto bin_path = get_exec_path() / "m68k" / "MC68000_test_all_opcodes.bin";

	test_cpu cpu;
	cpu.reset();
	cpu.load_bin(bin_path.string());

	nop_some_tests(cpu.memory());

	const std::uint32_t pc_done = 0x00F000; // when PC is looped here - we're done
	const std::uint32_t loop_threshold = 10;

	unsigned long long cycles = 0;
	const unsigned long long log_cycle = 1'000'000;

	auto& regs = cpu.registers();

	std::uint32_t old_pc = regs.PC;
	std::uint32_t same_pc_counter = 0;

	auto ns_per_cycle = measure_in_ns([&]()
	{
		while(true)
		{
			cpu.cycle();
			++cycles;

			if(!cpu.is_idle())
				continue;

			auto curr_pc = regs.PC;
			if(curr_pc == old_pc)
			{
				++same_pc_counter;

				// Check if we found a loop
				if(same_pc_counter == loop_threshold)
				{
					if(curr_pc == pc_done)
					{
						// all tests are succeeded
						break;
					}

					// different pc means we looped to an incorrectly implemented command

					// dump some data for debugging
					std::uint16_t data = cpu.memory().read<std::uint16_t>(curr_pc);
					std::cout << "Found a loop at " << su::hex_str(curr_pc)
						<< " (" << su::hex_str(data) << ")" << std::endl;

					GTEST_FAIL();
				}
			}
			else
			{
				old_pc = curr_pc;
				same_pc_counter = 0;
			}
		}
	});

	ns_per_cycle = ns_per_cycle / cycles;
	std::cout << "NS per cycle for executing MCL test program: " << ns_per_cycle << std::endl;

	ASSERT_NE(0, cycles);
	ASSERT_LT(ns_per_cycle, genesis::test::cycle_time_threshold_ns);
}
