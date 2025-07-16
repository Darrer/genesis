#ifndef __M68K_TEST_CPU_HPP__
#define __M68K_TEST_CPU_HPP__

#include "int_dev.h"
#include "m68k/cpu.h"
#include "memory/memory_unit.h"

#include <fstream>

namespace genesis::test
{

// const auto clock_rate = 16.67 /* MHz */ * 1'000'000;
constexpr auto clock_rate = 7.67 /* MHz */ * 1'000'000;
// For emulator performance, we need a more realistic threshold
// Real hardware: 1'000'000'000 / clock_rate = ~130.37 ns
// Emulator threshold: allow 10x slower for reasonable performance
constexpr auto cycle_time_threshold_ns = (1'000'000'000 / clock_rate) * 10;

const std::uint16_t nop_opcode = 0b0100111001110001;

// TODO: add m68k namespace or prefix
class test_cpu : public genesis::m68k::cpu
{
private:
	test_cpu(std::shared_ptr<memory::memory_unit> mem_unit, std::shared_ptr<int_dev> int_dev)
		: cpu(mem_unit, int_dev), mem_unit(mem_unit), _int_dev(int_dev)
	{
		if(exman.is_raised(m68k::exception_type::reset))
			exman.accept(m68k::exception_type::reset);
	}

public:
	test_cpu()
		: test_cpu(std::make_shared<memory::memory_unit>(0x1000000, std::endian::big), std::make_shared<int_dev>())
	{
	}

	memory::memory_unit& memory()
	{
		return *mem_unit;
	}

	m68k::bus_scheduler& bus_scheduler()
	{
		return scheduler;
	}

	m68k::exception_manager& exception_manager()
	{
		return exman;
	}

	m68k::bus_manager& bus_manager()
	{
		return busm;
	}

	int_dev& interrupt_dev()
	{
		return *_int_dev;
	}

	unsigned long long cycle_till_idle(unsigned long long cycles_limit = 1000)
	{
		return cycle_until([&]() { return is_idle(); }, cycles_limit);
	}

	// do cycle() untill func returns true
	template <class Callable>
	unsigned long long cycle_until(Callable&& func, unsigned long long cycles_limit = 1000)
	{
		unsigned long long cycles = 0;
		do
		{
			cycle();
			++cycles;

			if(cycles_limit != 0 && cycles > cycles_limit)
				throw std::runtime_error("cycle limit exceeded");

			bool stop = func() == true;
			if(stop)
				break;

		} while(true);

		return cycles;
	}

	void load_bin(std::string bin_path)
	{
		std::ifstream fs(bin_path, std::ios_base::binary);
		if(!fs.is_open())
			throw std::runtime_error("test_cpu::load_bin error: failed to open file '" + bin_path + "'");

		std::uint32_t offset = 0x0;
		while(fs)
		{
			char c;
			if(fs.get(c))
			{
				mem_unit->write(offset, c);
				++offset;
			}
		}
	}

private:
	std::shared_ptr<memory::memory_unit> mem_unit;
	std::shared_ptr<int_dev> _int_dev;
};

} // namespace genesis::test


#endif // __M68K_TEST_CPU_HPP__
