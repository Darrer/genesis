#include "mcl.h"
#include "time_utils.h"

#include <gtest/gtest.h>

using namespace genesis;
using namespace genesis::test;
using namespace genesis::m68k;


TEST(M68K, MCL)
{
	test_cpu cpu;

	long cycles = 0;
	bool succeed = false;
	auto total_ns_time = time::measure_in_ns([&]() { succeed = run_mcl(cpu, [&cycles]() { ++cycles; }); });

	auto ns_per_cycle = total_ns_time / cycles;
	std::cout << "NS per cycle for executing MCL test program: " << ns_per_cycle << ", total cycles: " << cycles
			  << std::endl;

	ASSERT_TRUE(succeed);
	ASSERT_NE(0, cycles);
	ASSERT_LT(ns_per_cycle, genesis::test::cycle_time_threshold_ns);
}
