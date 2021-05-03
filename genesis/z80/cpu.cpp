#include "cpu.h"

#include "string_utils.hpp"

// units
#include "impl/arithmetic_logic_unit.hpp"
#include "impl/cpu_unit.hpp"


namespace genesis::z80
{

cpu::cpu(std::shared_ptr<z80::memory> memory) : mem(memory)
{
	units.push_back(std::make_shared<unit::arithmetic_logic_unit>(*this));
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
	return std::any_of(units.begin(), units.end(), [](auto& u) { return u->execute(); });
}

} // namespace genesis::z80
