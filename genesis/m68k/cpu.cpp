#include "cpu.h"

#include "impl/instruction_unit.hpp"

namespace genesis::m68k
{

cpu::cpu(std::shared_ptr<m68k::memory> memory) : mem(memory)
{
	busm = std::make_unique<m68k::bus_manager>(_bus, regs, *mem, exman);
	pq = std::make_unique<m68k::prefetch_queue>(*busm, regs);
	auto inst = std::make_unique<m68k::instruction_unit>(regs, *busm, *pq);
	excp_handler = std::make_unique<m68k::exception_unit>(regs, *busm, *pq, exman, *inst);
	inst_unit = std::move(inst);
}

cpu::~cpu()
{
}

void cpu::cycle()
{
	// only instruction or exception cycle
	const bool exception_cycle = excp_handler->has_work() || !excp_handler->is_idle();
	if(exception_cycle)
		excp_handler->cycle();
	else
		inst_unit->cycle();

	pq->cycle();
	busm->cycle();
}

bool cpu::is_idle() const
{
	return inst_unit->is_idle() && pq->is_idle()
		&& busm->is_idle() && excp_handler->is_idle();
}

}
