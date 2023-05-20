#include <gtest/gtest.h>

#include "test_cpu.hpp"

using namespace genesis;


std::uint32_t wait_idle(test::test_cpu& cpu)
{
	m68k::bus_scheduler& scheduler = cpu.bus_scheduler();
	m68k::bus_manager& busm = cpu.bus_manager();

	std::uint32_t cycles = 0;
	while (!scheduler.is_idle() || !busm.is_idle())
	{
		scheduler.cycle();
		busm.cycle();
		++cycles;
	}

	return cycles;
}

void prepare_cpu(test::test_cpu& cpu, std::uint32_t PC, std::uint16_t IRD, std::uint16_t IRC)
{
	auto& regs = cpu.registers();

	regs.PC = PC;
	regs.IRD = regs.IR = IRD;
	regs.IRC = IRC;

	auto& mem = cpu.memory();

	mem.write(regs.PC, regs.IRD);
	mem.write(regs.PC + 2, regs.IRC);
}

const std::uint32_t expected_fetch_cycles = 4;

TEST(M68K_PREFETCH_QUEUE, FETCH_IRD)
{
	test::test_cpu cpu;

	std::uint16_t irc = 42;
	std::uint16_t ird = 24;
	std::uint32_t pc = 0x100;
	
	prepare_cpu(cpu, pc, ird, irc);
	
	auto& regs = cpu.registers();
	regs.IR = regs.IRD = 0;

	cpu.bus_scheduler().prefetch_ird();
	auto actual_fetch_cycles = wait_idle(cpu);

	ASSERT_EQ(expected_fetch_cycles, actual_fetch_cycles);
	ASSERT_EQ(pc, regs.PC);
	ASSERT_EQ(ird, regs.IRD);
	ASSERT_EQ(ird, regs.IR);
	ASSERT_EQ(irc, regs.IRC);
}

TEST(M68K_PREFETCH_QUEUE, FETCH_IRC)
{
	test::test_cpu cpu;

	std::uint16_t irc = 42;
	std::uint16_t ird = 24;
	std::uint32_t pc = 0x100;
	
	prepare_cpu(cpu, pc, ird, irc);
	
	auto& regs = cpu.registers();
	regs.IRC = 0;

	cpu.bus_scheduler().prefetch_irc();
	auto actual_fetch_cycles = wait_idle(cpu);

	ASSERT_EQ(expected_fetch_cycles, actual_fetch_cycles);
	ASSERT_EQ(pc, regs.PC);
	ASSERT_EQ(ird, regs.IRD);
	ASSERT_EQ(ird, regs.IR);
	ASSERT_EQ(irc, regs.IRC);
}

TEST(M68K_PREFETCH_QUEUE, FETCH_ONE)
{
	test::test_cpu cpu;

	std::uint16_t irc = 42;
	std::uint16_t old_irc = 88;
	std::uint16_t ird = 24;
	std::uint32_t pc = 0x100;
	
	prepare_cpu(cpu, pc, ird, irc);
	

	auto& regs = cpu.registers();
	regs.IRC = old_irc;

	cpu.bus_scheduler().prefetch_one();
	auto actual_fetch_cycles = wait_idle(cpu);

	ASSERT_EQ(expected_fetch_cycles, actual_fetch_cycles);
	ASSERT_EQ(pc, regs.PC);
	ASSERT_EQ(old_irc, regs.IRD);
	ASSERT_EQ(old_irc, regs.IR);
	ASSERT_EQ(irc, regs.IRC);
}

TEST(M68K_PREFETCH_QUEUE, INTERRUPT_CYCLE_THROW)
{
	test::test_cpu cpu;
	m68k::prefetch_queue pq(cpu.bus_manager(), cpu.registers());

	pq.init_fetch_one();

	ASSERT_THROW(pq.init_fetch_ird(), std::runtime_error);
	ASSERT_THROW(pq.init_fetch_irc(), std::runtime_error);
	ASSERT_THROW(pq.init_fetch_one(), std::runtime_error);
}
