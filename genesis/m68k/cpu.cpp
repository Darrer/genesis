#include "cpu.h"

#include "impl/instruction_unit.hpp"

namespace genesis::m68k
{

cpu::cpu(std::shared_ptr<m68k::memory> memory) : mem(memory)
{
	busm = std::make_unique<m68k::bus_manager>(_bus, regs, *mem, exman);
	pq = std::make_unique<m68k::prefetch_queue>(*busm, regs);
	scheduler = std::make_unique<m68k::bus_scheduler>(regs, *busm, *pq);
	inst_unit = std::make_unique<m68k::instruction_unit>(regs, exman, _bus, *busm, *scheduler);

	auto abort_execution = [this]()
	{
		pq->reset();
		inst_unit->reset();
		scheduler->reset();
	};
	
	auto instruction_unit_is_idle = [this]()
	{
		return inst_unit->is_idle();
	};

	excp_unit = std::make_unique<m68k::exception_unit>(regs, exman, *pq, *scheduler,
		abort_execution, instruction_unit_is_idle);
}

cpu::~cpu()
{
}

void cpu::cycle()
{
	// only instruction or exception cycle
	if(!excp_unit->is_idle())
	{
		excp_unit->cycle();
	}
	else
	{
		inst_unit->cycle();
	}

	scheduler->cycle();
	busm->cycle();

	scheduler->post_cycle();
	excp_unit->post_cycle();
	inst_unit->post_cycle();
}

bool cpu::is_idle() const
{
	// TODO: add exman?
	return busm->is_idle() && pq->is_idle() && scheduler->is_idle()
		&& inst_unit->is_idle() && excp_unit->is_idle();
}

}
