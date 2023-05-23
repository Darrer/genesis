#include <gtest/gtest.h>

#include <iostream>
#include <chrono>

#include "../test_cpu.hpp"

using namespace genesis::m68k;

template<class Callable>
long long measure_in_ns(const Callable& fn)
{
	auto start = std::chrono::high_resolution_clock::now();

	fn();

	auto stop = std::chrono::high_resolution_clock::now();

	auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
	return dur.count();
}

TEST(M68K_PERFORMANCE, TMP)
{
	GTEST_SKIP();

	const std::size_t num_copies = 1'000'000;

	std::size_t num_calls = 0;
	auto callback = [&num_calls]()
	{
		++num_calls;
	};

	auto time = measure_in_ns([&]()
	{
		for(std::size_t i = 0; i < num_copies; ++i)
		{
			std::function<void()> tmp = callback;
			// tmp();
			// callback();
		}
	});

	std::cout << "NS per copy: " << time / num_copies << std::endl;
	ASSERT_EQ(num_copies, num_calls);
}

TEST(M68K_PERFORMANCE, DECODING)
{
	GTEST_SKIP();

	const unsigned num_measurements = 100'000;

	int num_unknown = 0; // to prevent optimization

	auto ns_per_decode = measure_in_ns([&]()
	{
		for(unsigned i = 0; i < num_measurements; ++i)
		{
			std::uint16_t opcode = 0;
			while (true)
			{
				auto res = opcode_decoder::decode(opcode);
				
				if(opcode == 0xFFFF)
					break;
				++opcode;

				if(res == inst_type::NONE)
					++num_unknown;
			}
		}
	}) / 0x10000 / num_measurements;

	// NOTE: ns_per_decode is 1 or 0
	std::cout << "NS per decode: " << ns_per_decode << ", unknown: " << num_unknown << std::endl;
}

TEST(M68K_PERFORMANCE, BUS_READ)
{
	genesis::test::test_cpu cpu;

	const unsigned long long num_reads = 10'000'000;
	const unsigned long long num_cycles = num_reads * 4;

	unsigned long long num_callbacks = 0;
	// auto on_read_finish = [&num_callbacks]()
	auto on_read_finish = [&num_callbacks](std::uint32_t, size_type)
	{
		++num_callbacks;
	};

	auto& busm = cpu.bus_manager();
	auto& scheduler = cpu.bus_scheduler();
	unsigned long long cycles = 0;

	auto ns_per_cycle = measure_in_ns([&]()
	{
		for(auto i = 0; i < num_reads; ++i)
		{
			// busm.init_read_word(0, genesis::m68k::addr_space::PROGRAM);
			// while (!busm.is_idle())
			// {
			// 	busm.cycle();
			// 	++cycles;
			// }

			// scheduler.read(0, size_type::WORD, nullptr);//on_read_finish);
			scheduler.read(0, size_type::WORD, on_read_finish);
			while (!busm.is_idle() || !scheduler.is_idle())
			{
				scheduler.cycle();
				busm.cycle();
				++cycles;
			}
		}
	}) / num_cycles;

	// divide by 3 to have some cpu capacity
	const auto test_threshold_ns = genesis::test::cycle_time_threshold_ns / 3;

	// Takes 10-20 ns per cycle for bus_manager read operation
	// Takes ~37 ns per cycle for scheduler read operation
	std::cout << "NS per cycle for read operation: " << ns_per_cycle << ", threshold: " << test_threshold_ns << std::endl;

	ASSERT_EQ(num_cycles, cycles);
	ASSERT_EQ(num_reads, num_callbacks);
	ASSERT_LT(ns_per_cycle, test_threshold_ns);
}

TEST(M68K_PERFORMANCE, NOP)
{
	const unsigned long long num_executions = 10'000'000;

	const std::uint16_t nop_opcode = 0b0100111001110001;
	const unsigned long long num_cycles = num_executions * 4; // NOP takes 4 cycles to execute

	// const std::uint16_t nop_opcode = 0b0100111001110000; // RESET
	// const unsigned long long num_cycles = num_executions * 132; // RESET

	genesis::test::test_cpu cpu;

	// prepare mem
	auto& mem = cpu.memory();
	for(std::size_t i = 0; i < mem.max_capacity; i += 2)
		mem.write(i, nop_opcode);

	// disable tracing
	auto& regs = cpu.registers();
	regs.flags.TR = 0;
	regs.flags.S = 1;
	regs.PC = 0;
	regs.IR = regs.IRC = regs.IRD = nop_opcode;

	unsigned long long cycles_left = num_cycles;

	auto ns_per_cycle = measure_in_ns([&]()
	{
		while (cycles_left-- != 0)
			cpu.cycle();
	}) / num_cycles;

	// divide by 2 to have some cpu capacity
	const auto test_threshold_ns = genesis::test::cycle_time_threshold_ns / 2;

	std::cout << "NS per cycle for NOP: " << ns_per_cycle << ", threshold: " << test_threshold_ns << std::endl;

	ASSERT_TRUE(cpu.is_idle());
	ASSERT_LT(ns_per_cycle, test_threshold_ns);
}
