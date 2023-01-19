#include "cpu.h"

#include "impl/executioner.hpp"
#include "string_utils.hpp"


namespace genesis::z80
{

cpu::cpu(std::shared_ptr<z80::memory> memory) : mem(memory)
{
}

void cpu::execute_one()
{
	executioner::execute_one(*this);
}

} // namespace genesis::z80
