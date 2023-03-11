#include "cpu.h"


namespace genesis::m68k
{

cpu::cpu(std::shared_ptr<m68k::memory> memory) : mem(memory)
{
	busm = std::make_unique<m68k::bus_manager>(_bus, *mem);
	pq = std::make_unique<m68k::prefetch_queue>(*busm, regs);
	inst_handler = std::make_unique<m68k::instruction_handler>(regs, *busm, *pq);
}

cpu::~cpu()
{
}

void cpu::cycle()
{
	inst_handler->cycle();
	pq->cycle();
	busm->cycle();
}

bool cpu::is_idle() const
{
	return inst_handler->is_idle() && pq->is_idle() && busm->is_idle();
}

std::uint32_t cpu::execute_one()
{
	if(!is_idle())
		throw std::runtime_error("cpu::execute_one error: cannot execute one while in the middle of execution");

	std::uint32_t cycles = 0;
	do
	{
		cycle();
		++cycles;
	} while (!is_idle());

	return cycles;
}

}
