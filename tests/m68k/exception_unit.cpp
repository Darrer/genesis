#include "exception.hpp"
#include "m68k/impl/exception_manager.h"
#include "test_cpu.hpp"
#include "helpers/random.h"

#include <gtest/gtest.h>
#include <iostream>

// TODO: add test cases with rising exception during processing another exception


using namespace genesis::m68k;
using namespace genesis::test;

void clean_vector_table(genesis::memory::memory_unit& mem)
{
	for(int addr = 0; addr <= 1024; ++addr)
		mem.write(addr, std::uint8_t(0));
}

void rise_interrupt(test_cpu& cpu, std::uint8_t priority = 1, std::uint8_t IPM = 0)
{
	auto& regs = cpu.registers();

	regs.SSP.LW = 2048;
	regs.flags.IPM = IPM;

	cpu.set_interrupt(priority);
}

void rise_exception(test_cpu& cpu, exception_type ex)
{
	exception_manager& exman = cpu.exception_manager();
	switch(ex)
	{
	case exception_type::address_error:
	case exception_type::bus_error:
		exman.rise_address_error({0, 0, false, false});
		break;

	case exception_type::trace:
		exman.rise_trace();
		break;

	case exception_type::interrupt:
	{
		rise_interrupt(cpu);
		break;
	}

	default:
		exman.rise(ex);
		break;
	}
}

void setup_interrupt(test_cpu& cpu, std::uint8_t priority = 1, std::uint8_t IPM = 0)
{
	clean_vector_table(cpu.memory());
	rise_interrupt(cpu, priority, IPM);
}

void check_timings(exception_type ex, std::uint8_t expected_cycles)
{
	genesis::test::test_cpu cpu;
	auto& exman = cpu.exception_manager();

	clean_vector_table(cpu.memory());
	cpu.registers().SSP.LW = 2048;

	rise_exception(cpu, ex);

	ASSERT_TRUE(exman.is_raised(ex));

	auto cycles = cpu.cycle_till_idle();

	ASSERT_FALSE(exman.is_raised(ex));
	ASSERT_FALSE(exman.is_raised_any());
	ASSERT_EQ(expected_cycles, cycles);
}


TEST(M68K_EXCEPTION_UNIT, RESET_EXCEPTION_TIMINGS)
{
	check_timings(exception_type::reset, 40);
}

TEST(M68K_EXCEPTION_UNIT, ADDRESS_ERROR_TIMINGS)
{
	// TODO: why 50 - 1?
	check_timings(exception_type::address_error, 50 - 1);
}

TEST(M68K_EXCEPTION_UNIT, TRACE_TIMINGS)
{
	check_timings(exception_type::trace, 34);
}

TEST(M68K_EXCEPTION_UNIT, ILLIGAL_TIMINGS)
{
	check_timings(exception_type::illegal_instruction, 34);
	check_timings(exception_type::line_1010_emulator, 34);
	check_timings(exception_type::line_1111_emulator, 34);
}

const std::uint8_t interrupt_cycles = 44;

TEST(M68K_EXCEPTION_UNIT, INTERRUPT_TIMINGS)
{
	check_timings(exception_type::interrupt, interrupt_cycles);
}

TEST(M68K_EXCEPTION_UNIT, INTERRUPT_VALIDATE_STACK)
{
	// Setup
	test_cpu cpu;
	auto& regs = cpu.registers();
	auto& mem = cpu.memory();

	const std::uint8_t priority = (random::next<std::uint8_t>() % 7) + 1;
	setup_interrupt(cpu, priority);

	const std::uint32_t initial_sp = 2048;
	regs.SSP.LW = initial_sp;

	regs.SR = random::next<std::uint16_t>();
	regs.flags.IPM = 0;
	const std::uint16_t initial_sr = regs.SR;

	const std::uint32_t initial_pc = random::next<std::uint32_t>();
	regs.PC = initial_pc;

	// Act
	auto cycles = cpu.cycle_till_idle();
	ASSERT_EQ(cycles, interrupt_cycles);

	// Assert
	const std::uint32_t expected_sp = initial_sp - 2 /* SR */ - 4 /* PC */;
	ASSERT_EQ(expected_sp, regs.SSP.LW);

	const std::uint16_t pushed_sr = mem.read<std::uint16_t>(initial_sp - 2);
	ASSERT_EQ(initial_sr, pushed_sr);

	const std::uint32_t pushed_pc = mem.read<std::uint32_t>(initial_sp - 6);
	EXPECT_EQ(initial_pc, pushed_pc);

	// check status flag
	ASSERT_EQ(priority, regs.flags.IPM);
	ASSERT_EQ(1, regs.flags.S);
	ASSERT_EQ(0, regs.flags.TR);
}

void prepare_vector_table(test_cpu& cpu, std::uint32_t address, std::uint32_t pc_value, std::uint16_t ird, std::uint16_t irc)
{
	auto& mem = cpu.memory();

	clean_vector_table(mem);

	mem.write(address, pc_value);
	mem.write(pc_value, ird);
	mem.write(pc_value + 2, irc);
}

std::uint32_t random_pc(test_cpu& cpu)
{
	auto max_address = cpu.memory().max_address();
	std::uint32_t pc = random::next<std::uint32_t>() % max_address;

	if(pc % 2 == 1)
	{
		++pc; // PC must be even
	}

	if(pc < 1024)
	{
		pc += 1024; // PC should not point to vector table
	}

	std::uint32_t diff = max_address - pc;
	if(diff < 4)
	{
		pc -= 4; // there should be at least 4 bytes for prefetch queue (IRD/IRC)
	}

	return pc;
}

std::vector<std::uint32_t> build_interrupt_vector_addresses()
{
	std::vector<std::uint32_t> vectors;

	vectors.push_back(60); // Uninitialized Interrupt Vector
	vectors.push_back(96); // Spurious Interrupt
	
	// Autovectors
	for(std::uint32_t vec = 100; vec <= 124; vec += 4)
		vectors.push_back(vec);
	
	// User vectors
	for(std::uint32_t vec = 256; vec <= 1020; vec += 4)
		vectors.push_back(vec);

	return vectors;
}

std::uint8_t setup_int_device(test_cpu& cpu, std::uint32_t vector_addr)
{
	auto& dev = cpu.interrupt_dev();

	std::uint8_t priority = 1;

	if(vector_addr == 60)
	{
		dev.set_uninitialized();
	}
	else if(vector_addr == 96)
	{
		dev.set_spurious();
	}
	else if(vector_addr >= 100 && vector_addr <= 124)
	{
		priority = (vector_addr - 100) / 4 + 1;
		dev.set_autovectored();
	}
	else if(vector_addr >= 256 && vector_addr <= 1024)
	{
		dev.set_vectored(vector_addr / 4);
	}
	else
	{
		throw std::invalid_argument("vector_addr");
	}

	return priority;
}

TEST(M68K_EXCEPTION_UNIT, INTERRUPT_VECTORS_VALIDATE_PC)
{
	// Setup
	test_cpu cpu;
	auto& regs = cpu.registers();

	auto vec_addresses = build_interrupt_vector_addresses();
	for(auto addr : vec_addresses)
	{
		const std::uint32_t expected_pc = random_pc(cpu);
		const std::uint16_t expected_ird = random::next<std::uint16_t>();
		const std::uint16_t expected_irc = random::next<std::uint16_t>();

		prepare_vector_table(cpu, addr, expected_pc, expected_ird, expected_irc);

		auto priority = setup_int_device(cpu, addr);
		rise_interrupt(cpu, priority);

		// Act
		auto cycles = cpu.cycle_till_idle();
		ASSERT_EQ(cycles, interrupt_cycles);

		// Assert
		ASSERT_EQ(expected_pc, regs.PC);
		ASSERT_EQ(expected_ird, regs.IRD);
		ASSERT_EQ(expected_irc, regs.IRC);
	}
}

TEST(M68K_EXCEPTION_UNIT, INTERRUPT_UNMASKED)
{
	// Setup
	test_cpu cpu;
	setup_interrupt(cpu, 0b111, 0b111);

	// Act
	auto cycles = cpu.cycle_till_idle();

	// Assert
	ASSERT_EQ(interrupt_cycles, cycles);
	ASSERT_FALSE(cpu.exception_manager().is_raised(exception_type::interrupt));
}

TEST(M68K_EXCEPTION_UNIT, INTERRUPT_MASKED)
{
	for(std::uint8_t priority = 1; priority <= 5; ++priority)
	{
		// Setup
		test_cpu cpu;
		auto& regs = cpu.registers();

		setup_interrupt(cpu, priority, 5);

		regs.PC = 1024;
		regs.IRC = regs.IRD = nop_opcode;

		// Act
		auto cycles = cpu.cycle_till_idle();

		// Assert
		ASSERT_EQ(4, cycles); // it takes 4 cycles to execute NOP
		ASSERT_TRUE(cpu.exception_manager().is_raised(exception_type::interrupt));
	}
}
