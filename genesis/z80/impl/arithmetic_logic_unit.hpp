#include "../cpu.h"

#include <cstddef>
#include <iostream>


namespace genesis::z80::impl
{

class arithmetic_logic_unit
{
public:
	static bool execute(genesis::z80::cpu& cpu)
	{
		auto& regs = cpu.registers();
		auto& mem = cpu.memory();
		opcode op = mem.read<opcode>(regs.PC);

		if (op == 0x86)
		{
			regs.main_set.A += mem.read<std::uint8_t>(regs.main_set.HL);
		}
		else
		{
			return false;
		}

		return true;
	}
};


} // namespace genesis::z80::impl
