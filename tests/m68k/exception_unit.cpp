#include <gtest/gtest.h>

#include "test_cpu.hpp"
#include "m68k/impl/exception_manager.h"

using namespace genesis::m68k;

void setup_exception_vectors(memory& mem)
{
	for(memory::address addr = 0x0; addr <= 1024; ++addr)
		mem.write(addr, std::uint8_t(0));
}


void check_timings(exception_type ex, std::uint8_t expected_cycles)
{
	genesis::test::test_cpu cpu;
	auto& exman = cpu.exception_manager();

	setup_exception_vectors(cpu.memory());
	cpu.registers().SSP.LW = 2048;

	if(ex == exception_type::address_error)
		exman.rise_address_error({ 0, 0, 0, false, false });
	else
		exman.rise(ex);

	ASSERT_TRUE(exman.is_raised(ex));

	auto cycles = cpu.cycle_till_idle();

	ASSERT_FALSE(exman.is_raised(ex));
	ASSERT_EQ(expected_cycles, cycles);
}


TEST(M68K_EXCEPTION_UNIT, ADDRESS_ERROR)
{
	check_timings(exception_type::address_error, 50);
}
