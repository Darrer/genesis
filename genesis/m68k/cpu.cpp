#include "cpu.h"

#include "impl/bus_manager.hpp"
#include "impl/prefetch_queue.hpp"

#include <iostream>


namespace genesis::m68k
{

cpu::cpu(std::shared_ptr<m68k::memory> memory) : mem(memory)
{
	busm = std::make_unique<m68k::bus_manager>(_bus, *mem);
	pq = std::make_unique<m68k::prefetch_queue>(*busm, regs);
	// exec = std::make_unique<executioner>(*this, *busm);
}

cpu::~cpu()
{
}

void cpu::cycle()
{
	// exec->cycle();
	busm->cycle();
}

std::uint32_t cpu::execute_one()
{
	return 0;

	// TODO
	/* if(!exec->is_idle())
		throw std::runtime_error("cpu::execute_one error: cannot execute one while in the middle of execution");

	std::uint32_t cycles = 0;
	do
	{
		cycle();
		++cycles;
	} while (!exec->is_idle());
	
	return cycles; */
}

}
