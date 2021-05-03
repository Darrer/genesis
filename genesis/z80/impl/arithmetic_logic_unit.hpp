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

		if (execute_add(regs, mem))
			return true;

		return false;
	}

private:
	bool execute_add(genesis::z80::cpu_registers& regs, genesis::z80::memory& mem)
	{
		size_t opcode_size = 1;

		if (is_next_opcode(0x86))
		{
			regs.main_set.A += mem.read<std::uint8_t>(regs.main_set.HL);
		}
		else if (is_next_opcode(0x87))
		{
			regs.main_set.A += regs.main_set.A;
		}
		else if (is_next_opcode(0x80))
		{
			regs.main_set.A += regs.main_set.B;
		}
		else if (is_next_opcode(0x81))
		{
			regs.main_set.A += regs.main_set.C;
		}
		else if (is_next_opcode(0x82))
		{
			regs.main_set.A += regs.main_set.D;
		}
		else if (is_next_opcode(0x83))
		{
			regs.main_set.A += regs.main_set.E;
		}
		else if (is_next_opcode(0x84))
		{
			regs.main_set.A += regs.main_set.F;
		}
		else if (is_next_opcode(0x85))
		{
			regs.main_set.A += regs.main_set.L;
		}
		else
		{
			return false;
		}

		regs.PC += opcode_size;

		return true;
	}
};


} // namespace genesis::z80
