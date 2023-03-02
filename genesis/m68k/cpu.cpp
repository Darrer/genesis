#include "cpu.h"

#include "impl/bus_manager.hpp"
#include "impl/prefetch_queue.hpp"
#include "impl/ea_decoder.hpp"
#include "impl/executioner.hpp"

#include <iostream>


namespace genesis::m68k
{

cpu::cpu(std::shared_ptr<m68k::memory> memory) : mem(memory)
{
	busm = std::make_unique<m68k::bus_manager>(_bus, *mem);
	pq = std::make_unique<m68k::prefetch_queue>(*busm, regs);
	decoder = std::make_unique<m68k::ea_decoder>(*busm, regs, *pq);
	exec = std::make_unique<executioner>(*this, *busm, *decoder);
}

cpu::~cpu()
{
}

void cpu::cycle()
{
	exec->cycle();
	decoder->cycle();
	pq->cycle();
	busm->cycle();
}

std::uint32_t cpu::execute_one()
{
	if(!exec->is_idle())
		throw std::runtime_error("cpu::execute_one error: cannot execute one while in the middle of execution");

	std::uint32_t cycles = 0;
	do
	{
		cycle();
		++cycles;
	} while (!exec->is_idle());

	return cycles;
}

}
