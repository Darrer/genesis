#ifndef __M68K_TEST_MCL_H__
#define __M68K_TEST_MCL_H__

#include "helper.hpp"
#include "m68k/test_cpu.hpp"

#include <gtest/gtest.h>

namespace genesis::test
{

namespace __impl
{

static inline void nop_some_tests(genesis::memory::memory_unit& mem)
{
	auto nop_test = [&](std::uint32_t jump_addr) {
		// jump takes 6 bytes - 2 for opcode, 4 for ptr
		mem.write(jump_addr, nop_opcode);
		mem.write(jump_addr + 2, nop_opcode);
		mem.write(jump_addr + 4, nop_opcode);
	};

	// TODO: ABCD/SBCD have tricky logic and I'm too lazy to figure it out,
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

static inline void load_mcl(test_cpu& cpu)
{
	const auto bin_path = get_exec_path() / "m68k" / "MC68000_test_all_opcodes.bin";

	cpu.reset();
	cpu.load_bin(bin_path.string());
}

} // namespace __impl

// The test program is taken from: https://github.com/MicroCoreLabs/Projects/blob/master/MCL68/MC68000_Test_Code/
template <class Callable>
bool run_mcl(test_cpu& cpu, Callable&& after_cycle_hook)
{
	__impl::load_mcl(cpu);
	__impl::nop_some_tests(cpu.memory());

	const std::uint32_t pc_done = 0x00F000; // when PC is looped here - we're done
	const std::uint32_t loop_threshold = 10;

	auto& regs = cpu.registers();

	std::uint32_t old_pc = regs.PC;
	std::uint32_t same_pc_counter = 0;

	// auto busy_cycles = 0ull;

	// set cycles threshold to prevent infinitive loops
	// it take a little bit less than 3 million cycles to execute MCL program,
	// but set threshold to much bigger value to account for tests that take control over cpu bus
	const auto cycles_threshld = 100'000'000;
	auto cycles = 0ull;

	while(true)
	{
		if(++cycles == cycles_threshld)
			throw internal_error("run_mcl exceed cycles limit");

		cpu.cycle();
		after_cycle_hook();

		if(testing::Test::HasFatalFailure())
			return false;

		if(!cpu.is_idle())
		{
			// ++busy_cycles;
			continue;
		}

		// if(busy_cycles > 50)
		// 	std::cout << "Busy cycles: " << busy_cycles << std::endl;
		// busy_cycles = 0;

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
					return true;
				}

				// different pc means we looped to an incorrectly implemented command

				// dump some data for debugging
				std::uint16_t data = cpu.memory().read<std::uint16_t>(curr_pc);
				std::cout << "Found a loop at " << su::hex_str(curr_pc) << " (" << su::hex_str(data) << ")"
						  << std::endl;

				return false;
			}
		}
		else
		{
			old_pc = curr_pc;
			same_pc_counter = 0;
		}
	}
}

} // namespace genesis::test

#endif // __M68K_TEST_MCL_H__
