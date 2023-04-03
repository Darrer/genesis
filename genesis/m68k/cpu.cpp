#include "cpu.h"

#include "impl/instruction_unit.hpp"

namespace genesis::m68k
{

cpu::cpu(std::shared_ptr<m68k::memory> memory) : mem(memory)
{
	busm = std::make_unique<m68k::bus_manager>(_bus, regs, *mem, exman);
	pq = std::make_unique<m68k::prefetch_queue>(*busm, regs);
	scheduler = std::make_unique<m68k::bus_scheduler>(regs, *busm, *pq);

	auto inst = std::make_unique<m68k::instruction_unit>(regs, *busm, *pq, *scheduler);
	excp_unit = std::make_unique<m68k::exception_unit>(regs, *busm, *pq, exman, *inst, *scheduler);
	inst_unit = std::move(inst);
}

cpu::~cpu()
{
}

void cpu::cycle()
{
	// only instruction or exception cycle
	const bool exception_cycle = !excp_unit->is_idle() || excp_unit->has_work();
	if(exception_cycle)
	{
		excp_unit->cycle();
	}
	else
	{
		inst_unit->cycle();
	}

	scheduler->cycle();
	pq->cycle();
	busm->cycle();

	excp_unit->post_cycle();
	inst_unit->post_cycle();

	// if exception of group 0 rised - do cycle now
	if(!exception_cycle && excp_unit->has_work())
		excp_unit->cycle();
}

bool cpu::is_idle() const
{
	return busm->is_idle() && pq->is_idle() && scheduler->is_idle()
		&& inst_unit->is_idle() && excp_unit->is_idle();
}

}
