#include "cpu.h"

#include "impl/executioner.hpp"
#include "string_utils.hpp"


namespace genesis::z80
{

cpu::cpu(std::shared_ptr<z80::memory> memory, std::shared_ptr<z80::io_ports> io_ports) : mem(memory), ports(io_ports)
{
	reset();
	exec = std::make_unique<z80::executioner>(*this);
}

cpu::~cpu()
{
}

void cpu::reset()
{
	regs.IFF1 = regs.IFF2 = 0;
	regs.I = regs.R = 0;
	regs.PC = 0;
	int_mode = cpu_interrupt_mode::im0;
}

void cpu::execute_one()
{
	exec->execute_one();
}

} // namespace genesis::z80
