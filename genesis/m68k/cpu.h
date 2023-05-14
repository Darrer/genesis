#ifndef __M68K_CPU_H__
#define __M68K_CPU_H__

#include "cpu_registers.hpp"
#include "cpu_bus.hpp"
#include "cpu_memory.h"

#include "impl/base_unit.h"

#include "impl/bus_manager.hpp"
#include "impl/prefetch_queue.hpp"
#include "impl/bus_scheduler.h"
#include "impl/exception_manager.h"
#include "impl/exception_unit.hpp"
#include "impl/trace_riser.hpp"

#include <memory>


namespace genesis::m68k
{

class cpu
{
public:
	cpu(std::shared_ptr<m68k::memory> memory);
	~cpu();

	cpu_registers& registers() { return regs; }
	cpu_bus& bus() { return _bus; }

	bool is_idle() const;
	void cycle();

protected:
	cpu_registers regs;
	cpu_bus _bus;
	std::shared_ptr<m68k::memory> mem;

	m68k::exception_manager exman;
	m68k::bus_manager busm;
	m68k::prefetch_queue pq;
	m68k::bus_scheduler scheduler;

	// TODO: do we really need pointers here?
	std::unique_ptr<m68k::base_unit> inst_unit;
	std::unique_ptr<m68k::exception_unit> excp_unit;
	std::unique_ptr<impl::trace_riser> tracer;
};

}

#endif // __M68K_CPU_H__
