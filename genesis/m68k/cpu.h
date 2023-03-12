#ifndef __M68K_CPU_H__
#define __M68K_CPU_H__

#include "cpu_registers.hpp"
#include "cpu_bus.hpp"

#include "impl/bus_manager.hpp"
#include "impl/prefetch_queue.hpp"
#include "impl/instruction_handler.hpp"
#include "impl/exception_handler.hpp"

#include <cstdint>
#include <memory>


namespace genesis::m68k
{

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

	// TODO: remove from public interface
	bus_manager& bus_manager()
	{
		return *busm;
	}

	// TODO: remove from public interface
	prefetch_queue& prefetch_queue()
	{
		return *pq;
	}

	memory& memory()
	{
		return *mem;
	}

	void cycle();

	// NOTE: only for unit tests
	bool is_idle() const;
	std::uint32_t execute_one();

private:
	cpu_registers regs;
	cpu_bus _bus;
	std::shared_ptr<m68k::memory> mem;

	std::unique_ptr<m68k::bus_manager> busm;
	std::unique_ptr<m68k::prefetch_queue> pq;
	std::unique_ptr<m68k::instruction_handler> inst_handler;
};


[[maybe_unused]]
static m68k::cpu make_cpu()
{
	return m68k::cpu(std::make_shared<m68k::memory>());
}

}

#endif // __M68K_CPU_H__
