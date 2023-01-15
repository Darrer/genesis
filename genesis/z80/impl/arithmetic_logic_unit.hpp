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
		case 0x87:
			add(regs, regs.main_set.A);
			break;
		case 0x80:
			add(regs, regs.main_set.B);
			break;
		case 0x81:
			add(regs, regs.main_set.C);
			break;
		case 0x82:
			add(regs, regs.main_set.D);
			break;
		case 0x83:
			add(regs, regs.main_set.E);
			break;
		case 0x84:
			add(regs, regs.main_set.H);
			break;
		case 0x85:
			add(regs, regs.main_set.L);
			break;
		case 0xC6:
			opcode_size = 2;
			add(regs, mem.read<z80::opcode>(regs.PC + 1));
			break;
		case 0x86:
			add(regs, mem.read<std::uint8_t>(regs.main_set.HL));
			break;
		case 0xDD:
		{
			opcode_size = 2;
			z80::opcode op2 = mem.read<z80::opcode>(regs.PC + 1);
			switch (op2)
			{
			case 0x86:
			{
				opcode_size = 3;
				z80::opcode d = mem.read<z80::opcode>(regs.PC + 2);
				add(regs, mem.read<z80::opcode>(regs.IX + d));
				break;
			}
			
			default:
				return false;
			}
			break;
		}
		case 0xFD:
		{
			opcode_size = 2;
			z80::opcode op2 = mem.read<z80::opcode>(regs.PC + 1);
			switch (op2)
			{
			case 0x86:
			{
				opcode_size = 3;
				z80::opcode d = mem.read<z80::opcode>(regs.PC + 2);
				add(regs, mem.read<z80::opcode>(regs.IY + d));
				break;
			}
			
			default:
				return false;
			}
			break;
		}
		default:
			return false;
		}

		regs.main_set.flags.S = (regs.main_set.A < 0) ? 1 : 0;
		regs.main_set.flags.Z = (regs.main_set.A == 0) ? 1 : 0;
		regs.main_set.flags.N = 0;

		regs.PC += opcode_size;

		return true;
	}

protected:
	/* 8-Bit Arithmetic Group */
	inline static void add(z80::cpu_registers& regs, std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;

		auto& flags = regs.main_set.flags;

		// check Half Carry Flag (H)
		flags.H = ((_a & 0x0f) + (_b & 0x0f) > 0xf) ? 1 : 0;

		// check Parity/Overflow Flag (P/V)
		flags.PV = (int)_a + (int)_b > 127 || (int)_a + (int)b < -128 ? 1 : 0;

		// check Carry Flag (C)
		flags.C = (int)_a + (int)_b > 0xff ? 1 : 0;

		regs.main_set.A += b;
	}
};


} // namespace genesis::z80
