#include "cpu.h"


namespace genesis::m68k
{

cpu::cpu(std::shared_ptr<m68k::memory> memory) : mem(memory)
{
	busm = std::make_unique<m68k::bus_manager>(_bus, *mem);
	pq = std::make_unique<m68k::prefetch_queue>(*busm, regs);
	inst_handler = std::make_unique<m68k::instruction_handler>(regs, *busm, *pq);
	excp_handler = std::make_unique<m68k::exception_handler>(regs, *busm, *pq);
}

cpu::~cpu()
{
}

void cpu::cycle()
{
	// only instruction or exception cycle
	// const bool 
	if(!excp_handler->is_idle())
		excp_handler->cycle();
	else
		inst_handler->cycle();

	pq->cycle();
	busm->cycle();
}

bool cpu::is_idle() const
{
	return inst_handler->is_idle() && pq->is_idle()
		&& busm->is_idle() && excp_handler->is_idle();
}

}
