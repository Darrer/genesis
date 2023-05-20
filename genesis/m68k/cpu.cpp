#include "cpu.h"

#include "impl/instruction_unit.hpp"

namespace genesis::m68k
{

cpu::cpu(std::shared_ptr<m68k::memory> memory) : mem(memory), busm(_bus, regs, *mem, exman), scheduler(regs, busm)
{
	inst_unit = std::make_unique<m68k::instruction_unit>(regs, exman, _bus, busm, scheduler);

	auto abort_execution = [this]()
	{
		inst_unit->reset();
		scheduler.reset();
		tracer->reset();
	};

	auto instruction_unit_is_idle = [this]()
	{
		return inst_unit->is_idle();
	};

	excp_unit = std::make_unique<m68k::exception_unit>(regs, exman, _bus, scheduler,
		abort_execution, instruction_unit_is_idle);
	
	tracer = std::make_unique<impl::trace_riser>(regs, exman, instruction_unit_is_idle);

	reset();
}

cpu::~cpu()
{
}

void cpu::reset()
{
	exman.rise(exception_type::reset);
}

void cpu::cycle()
{
	// TODO: move to instruction unit
	// tracer->cycle();

	// only instruction or exception cycle
	bool exception_cycle = !excp_unit->is_idle();
	if(exception_cycle)
	{
		excp_unit->cycle();
	}
	else
	{
		inst_unit->cycle();
	}

	scheduler.cycle();
	busm.cycle();

	if(exception_cycle)
	{
		excp_unit->post_cycle();
	}
	else
	{
		inst_unit->post_cycle();
	}

	// tracer->post_cycle();
}

bool cpu::is_idle() const
{
	// TODO: add exman?
	return busm.is_idle() && scheduler.is_idle()
		&& inst_unit->is_idle() && excp_unit->is_idle();
}

}
