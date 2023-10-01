#include "exception.hpp"
#include "m68k/impl/exception_manager.h"
#include "test_cpu.hpp"

#include <gtest/gtest.h>


using namespace genesis::m68k;

void setup_exception_vectors(genesis::memory::memory_unit& mem)
{
	for (int addr = 0; addr <= 1024; ++addr)
		mem.write(addr, std::uint8_t(0));
}

void rise_exception(exception_manager& exman, exception_type ex)
{
	switch (ex)
	{
	case exception_type::address_error:
	case exception_type::bus_error:
		exman.rise_address_error({0, 0, false, false});
		break;

	case exception_type::trace:
		exman.rise_trace();
		break;

	default:
		exman.rise(ex);
		break;
	}
}


void check_timings(exception_type ex, std::uint8_t expected_cycles)
{
	genesis::test::test_cpu cpu;
	auto& exman = cpu.exception_manager();

	setup_exception_vectors(cpu.memory());
	cpu.registers().SSP.LW = 2048;

	rise_exception(exman, ex);

	ASSERT_TRUE(exman.is_raised(ex));

	auto cycles = cpu.cycle_till_idle();

	ASSERT_FALSE(exman.is_raised(ex));
	ASSERT_EQ(expected_cycles, cycles);
}


TEST(M68K_EXCEPTION_UNIT, RESET_EXCEPTION)
{
	check_timings(exception_type::reset, 40);
}

TEST(M68K_EXCEPTION_UNIT, ADDRESS_ERROR)
{
	check_timings(exception_type::address_error, 50 - 1);
}

TEST(M68K_EXCEPTION_UNIT, TRACE)
{
	check_timings(exception_type::trace, 34);
}

TEST(M68K_EXCEPTION_UNIT, ILLIGAL)
{
	check_timings(exception_type::illegal_instruction, 34);
	check_timings(exception_type::line_1010_emulator, 34);
	check_timings(exception_type::line_1111_emulator, 34);
}
