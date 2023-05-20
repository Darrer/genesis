#ifndef __M68K_TEST_CPU_HPP__
#define __M68K_TEST_CPU_HPP__

#include "m68k/cpu.h"

namespace genesis::test
{

const auto clock_rate = 16.67 /* MHz */ * 1'000'000;
const auto cycle_time_threshold_ns = 1'000'000'000 / clock_rate;

class test_cpu : public genesis::m68k::cpu
{
public:
	test_cpu() : cpu(std::make_shared<m68k::memory>())
	{
		if(exman.is_raised(m68k::exception_type::reset))
			exman.accept(m68k::exception_type::reset);
	}

	m68k::memory& memory() { return *mem; }
	m68k::bus_manager& bus_manager() { return busm; }
	m68k::bus_scheduler& bus_scheduler() { return scheduler; }
	m68k::exception_manager& exception_manager() { return exman; }

	unsigned long long cycle_till_idle(unsigned long long cycles_limit = 1000)
	{
		unsigned long long cycles = 0;
		do
		{
			cycle();
			++cycles;

			if(cycles_limit != 0 && cycles > cycles_limit)
				break;
		} while (!is_idle());

		return cycles;
	}
};

}


#endif // __M68K_TEST_CPU_HPP__
