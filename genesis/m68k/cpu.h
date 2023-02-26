#ifndef __M68K_CPU_H__
#define __M68K_CPU_H__

#include "cpu_registers.hpp"
#include "cpu_bus.hpp"
#include "memory.hpp"

#include <cstdint>
#include <memory>


namespace genesis::m68k
{

using memory = genesis::memory<std::uint32_t, 0x1000000 /* 16 MiB */, std::endian::big>;

class bus_manager;
class prefetch_queue;

class cpu
{
public:
	cpu(std::shared_ptr<m68k::memory> memory);
	~cpu();

	cpu_registers& registers()
	{
		return regs;
	}

	cpu_bus& bus()
	{
		return _bus;
	}

	bus_manager& bus_manager()
	{
		return *busm;
	}

	prefetch_queue& prefetch_queue()
	{
		return *pq;
	}

	memory& memory()
	{
		return *mem;
	}

	void cycle();
	std::uint32_t execute_one();

private:
	cpu_registers regs;
	cpu_bus _bus;
	std::shared_ptr<m68k::memory> mem;

	std::unique_ptr<m68k::bus_manager> busm;
	std::unique_ptr<m68k::prefetch_queue> pq;
};


[[maybe_unused]]
static m68k::cpu make_cpu()
{
	return m68k::cpu(std::make_shared<m68k::memory>());
}

}

#endif // __M68K_CPU_H__
