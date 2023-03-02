#include <gtest/gtest.h>

#include "m68k/impl/prefetch_queue.hpp"
#include "m68k/impl/bus_manager.hpp"
#include "m68k/cpu_registers.hpp"


using namespace genesis;

#define setup_test() \
	m68k::cpu_bus bus; \
	auto mem = std::make_shared<m68k::memory>(); \
	m68k::bus_manager busm(bus, *mem); \
	m68k::cpu_registers regs; \
	m68k::prefetch_queue pq(busm, regs)


std::uint32_t wait_idle(m68k::prefetch_queue& pq, m68k::bus_manager& busm)
{
	std::uint32_t cycles = 0;
	while (!pq.is_idle() || !busm.is_idle())
	{
		pq.cycle();
		busm.cycle();
		++cycles;
	}

	return cycles;
}

std::uint32_t fetch_one(m68k::prefetch_queue& pq, m68k::bus_manager& busm)
{
	pq.init_fetch_one();
	return wait_idle(pq, busm);
}

std::uint32_t fetch_irc(m68k::prefetch_queue& pq, m68k::bus_manager& busm)
{
	pq.init_fetch_irc();
	return wait_idle(pq, busm);
}

std::uint32_t fetch_two(m68k::prefetch_queue& pq, m68k::bus_manager& busm)
{
	pq.init_fetch_two();
	return wait_idle(pq, busm);
}

const std::uint32_t expected_fetch_cycles = 4;

TEST(M68K_PREFETCH_QUEUE, FETCH_ONE)
{
	setup_test();

	std::uint16_t old_irc = 42;
	pq.IRC = old_irc;

	regs.PC = 0x101;
	std::uint16_t val = 42240;
	mem->write(regs.PC, val);

	auto actual_fetch_cycles = fetch_one(pq, busm);

	ASSERT_EQ(expected_fetch_cycles, actual_fetch_cycles);
	ASSERT_EQ(0x103, regs.PC);
	ASSERT_EQ(val, pq.IRC);

	// old irc should go to IR/IRD
	ASSERT_EQ(old_irc, pq.IR);
	ASSERT_EQ(old_irc, pq.IRD);
}

TEST(M68K_PREFETCH_QUEUE, FETCH_IRC)
{
	setup_test();

	std::uint16_t old_ir = 24;
	pq.IRC = 42;
	pq.IR = pq.IRD = old_ir;

	regs.PC = 0x101;
	std::uint16_t val = 42240;
	mem->write(regs.PC, val);

	auto actual_fetch_cycles = fetch_irc(pq, busm);

	ASSERT_EQ(expected_fetch_cycles, actual_fetch_cycles);
	ASSERT_EQ(0x103, regs.PC);
	ASSERT_EQ(val, pq.IRC);

	// these should not be changed
	ASSERT_EQ(old_ir, pq.IR);
	ASSERT_EQ(old_ir, pq.IRD);
}

TEST(M68K_PREFETCH_QUEUE, FETCH_TWO)
{
	setup_test();

	regs.PC = 0x101;
	std::uint16_t val = 42240;
	std::uint16_t val2 = 11666;
	mem->write(regs.PC, val);
	mem->write(regs.PC + 2, val2);

	auto actual_fetch_cycles = fetch_two(pq, busm);

	ASSERT_EQ(expected_fetch_cycles * 2, actual_fetch_cycles);
	ASSERT_EQ(0x105, regs.PC);
	ASSERT_EQ(val, pq.IRD);
	ASSERT_EQ(val, pq.IR);
	ASSERT_EQ(val2, pq.IRC);
}

TEST(M68K_PREFETCH_QUEUE, INTERRUPT_CYCLE_THROW)
{
	setup_test();

	pq.init_fetch_one();

	ASSERT_THROW(pq.init_fetch_one(), std::runtime_error);
	ASSERT_THROW(pq.init_fetch_two(), std::runtime_error);
	ASSERT_THROW(pq.init_fetch_irc(), std::runtime_error);
}
