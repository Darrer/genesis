#include "base_unit.hpp"

#include <cstddef>
#include <iostream>


namespace genesis::z80
{

class cpu::unit::arithmetic_logic_unit : public cpu::unit::base_unit
{
public:
	using base_unit::base_unit;

	bool execute() override
	{
		auto& regs = cpu.registers();
		auto& mem = cpu.memory();

		z80::opcode op = mem.read<z80::opcode>(regs.PC);

		if (execute_add(op, regs, mem))
			return true;

		return false;
	}

private:
	bool execute_add(z80::opcode op, genesis::z80::cpu_registers& regs, genesis::z80::memory& mem)
	{
		size_t opcode_size = 1;

		switch (op)
		{
		case 0x86:
			regs.main_set.A += mem.read<std::uint8_t>(regs.main_set.HL);
			break;
		case 0x87:
			regs.main_set.A += regs.main_set.A;
			break;
		case 0x80:
			regs.main_set.A += regs.main_set.B;
			break;
		case 0x81:
			regs.main_set.A += regs.main_set.C;
			break;
		case 0x82:
			regs.main_set.A += regs.main_set.D;
			break;
		case 0x83:
			regs.main_set.A += regs.main_set.E;
			break;
		case 0x84:
			regs.main_set.A += regs.main_set.F;
			break;
		case 0x85:
			regs.main_set.A += regs.main_set.L;
			break;
		case 0x88:

			break;
		default:
			return false;
		}

		regs.PC += opcode_size;

		return true;
	}
};


} // namespace genesis::z80
