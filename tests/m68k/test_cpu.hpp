#ifndef __M68K_TEST_CPU_HPP__
#define __M68K_TEST_CPU_HPP__

#include "m68k/cpu.h"
#include "memory/memory_unit.h"
#include <fstream>

namespace genesis::test
{

// const auto clock_rate = 16.67 /* MHz */ * 1'000'000;
const auto clock_rate = 7.670454 /* MHz */ * 1'000'000;
const auto cycle_time_threshold_ns = 1'000'000'000 / clock_rate;

class test_cpu : public genesis::m68k::cpu
{
private:
	test_cpu(std::shared_ptr<memory::memory_unit> mem_unit) : cpu(mem_unit), mem_unit(mem_unit)
	{
		if(exman.is_raised(m68k::exception_type::reset))
			exman.accept(m68k::exception_type::reset);
	}

public:
	test_cpu() : test_cpu(std::make_shared<memory::memory_unit>(0x1000000, std::endian::big)) { }

	memory::memory_unit& memory() { return *mem_unit; }
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

	void load_bin(std::string bin_path)
	{
		std::ifstream fs(bin_path, std::ios_base::binary);
		if (!fs.is_open())
			throw std::runtime_error("test_cpu::load_bin error: failed to open file '" + bin_path + "'");

		std::uint32_t offset = 0x0;
		while (fs)
		{
			char c;
			if (fs.get(c))
			{
				mem_unit->write(offset, c);
				++offset;
			}
		}
	}

private:
	std::shared_ptr<memory::memory_unit> mem_unit;
};

}


#endif // __M68K_TEST_CPU_HPP__
