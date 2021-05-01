#include "cpu.h"

#include "string_utils.hpp"

// units
#include "impl/arithmetic_logic_unit.hpp"


namespace genesis::z80
{

cpu::cpu(std::shared_ptr<z80_mem> memory) : mem(memory)
{
}


void cpu::execute_one()
{
	bool executed = try_execute();

	if (!executed)
	{
		throw std::runtime_error("Execute failed, unsupported opcode at: " + su::hex_str(regs.PC));
	}
}


bool cpu::try_execute()
{
	using namespace impl;

	if (arithmetic_logic_unit::execute(*this))
		return true;

	return false;
}

} // namespace genesis::z80
