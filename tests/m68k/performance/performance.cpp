#include <gtest/gtest.h>

#include <iostream>
#include <chrono>

#include "../test_cpu.hpp"

using namespace genesis::m68k;

TEST(M68K, THT_MAP_PERFORMANCE)
{
	const unsigned int num_measurements = 1'000'000;

	std::uint16_t num_unknown = 0; // to prevent optimization


	auto start = std::chrono::high_resolution_clock::now();

	for(auto i = 0; i < num_measurements; ++i)
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

	auto stop = std::chrono::high_resolution_clock::now();

	auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);

	auto ns_per_decode = dur.count() / 0x10000 / num_measurements;

	// NOTE: ns_per_decode is 1 or 0
	std::cout << "NS per decode: " << ns_per_decode << ", unknown: " << num_unknown << std::endl;
}

TEST(M68K_PERFORMANCE, BUS_READ)
{
	genesis::test::test_cpu cpu;

	const unsigned long long num_reads = 10'000'000;
	const unsigned long long num_cycles = num_reads * 4;

	unsigned long long num_callbacks = 0;
	auto on_read_finish = [&num_callbacks](std::uint32_t, size_type)
	{
		++num_callbacks;
	};

	auto start = std::chrono::high_resolution_clock::now();

	auto& busm = cpu.bus_manager();
	auto& scheduler = cpu.bus_scheduler();
	unsigned long long cycles = 0;
	for(auto i = 0; i < num_reads; ++i)
	{
		// busm.init_read_word(0, genesis::m68k::addr_space::PROGRAM, on_read_finish);
		scheduler.read(0, size_type::WORD, on_read_finish);
		while (!busm.is_idle() || !scheduler.is_idle())
		{
			scheduler.cycle();
			busm.cycle();
			scheduler.post_cycle();
			++cycles;
		}
	}

	auto stop = std::chrono::high_resolution_clock::now();

	auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
	auto ns_per_cycle = dur.count() / num_cycles;
	
	// Takes 10-20 ns per cycle for read operation
	std::cout << "Reading takes " << ns_per_cycle << " ns per cycle, total reads: " << num_callbacks << std::endl;
	std::cout << "Operation size: " << sizeof(genesis::m68k::bus_scheduler::operation) << std::endl;
	ASSERT_EQ(num_cycles, cycles);
}

TEST(M68K_PERFORMANCE, NOP)
{
	const unsigned long long num_executions = 1'000'000;

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
	auto start = std::chrono::high_resolution_clock::now();

	while (cycles_left-- != 0)
		cpu.cycle();

	auto stop = std::chrono::high_resolution_clock::now();

	auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
	auto ns_per_cycle = dur.count() / num_cycles;

	// It takes 60-65 ns
	std::cout << "NS per cycle for NOP: " << ns_per_cycle << ", PC: " << cpu.registers().PC
		<< ", threshold: " << genesis::test::cycle_time_threshold_ns << std::endl;

	ASSERT_TRUE(cpu.is_idle());
	ASSERT_LT(ns_per_cycle, genesis::test::cycle_time_threshold_ns);
}
