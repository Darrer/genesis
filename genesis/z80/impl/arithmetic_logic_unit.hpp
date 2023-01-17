#include "base_unit.hpp"

#include <cstddef>
#include <iostream>
#include <cassert>


namespace genesis::z80
{

class cpu::unit::arithmetic_logic_unit : public cpu::unit::base_unit
{
public:
	using base_unit::base_unit;

	bool execute() override
	{
		auto& regs = cpu.registers();

		z80::opcode op = fetch_pc();

		if (execute_add(op, regs))
			return true;
		if (execute_adc(op, regs))
			return true;
		if(execute_sub(op, regs))
			return true;
		if(execute_sbc(op, regs))
			return true;
		if(execute_and_8(op, regs))
			return true;

		return false;
	}

private:
	bool execute_add(z80::opcode op, z80::cpu_registers& regs)
	{
		size_t opcode_size = 1;

		switch (op)
		{
		case 0x87:
		case 0x80:
		case 0x81:
		case 0x82:
		case 0x83:
		case 0x84:
		case 0x85:
			add(regs, r(op & 0b111, regs));
			break;
		case 0xC6:
			opcode_size = 2;
			add(regs, fetch_pc<std::uint8_t>(+1));
			break;
		case 0x86:
			add(regs, fetch_hl<std::uint8_t>());
			break;
		case 0xDD:
		case 0xFD:
		{
			if(fetch_pc(+1) == 0x86)
			{
				opcode_size = 3;
				auto d = fetch_pc<std::int8_t>(+2);
				auto base = idx(op, regs);

				add(regs, fetch<std::uint8_t>(base + d));
			}
			else
			{
				return false;
			}
			break;
		}

		default:
			return false;
		}

		regs.PC += opcode_size;

		return true;
	}

	bool execute_adc(z80::opcode op, z80::cpu_registers& regs)
	{
		size_t opcode_size = 1;

		switch (op)
		{
		case 0x8F:
		case 0x88:
		case 0x89:
		case 0x8A:
		case 0x8B:
		case 0x8C:
		case 0x8D:
			adc(regs, r(op & 0b111, regs));
			break;
		case 0xCE:
			opcode_size = 2;
			adc(regs, fetch_pc<std::uint8_t>(+1));
			break;
		case 0x8E:
			adc(regs, fetch_hl<std::uint8_t>());
			break;
		case 0xDD:
		case 0xFD:
		{
			if(fetch_pc(+1) == 0x8E)
			{
				opcode_size = 3;
				auto d = fetch_pc<std::int8_t>(+2);
				auto base = idx(op, regs);

				adc(regs, fetch<std::uint8_t>(base + d));
			}
			else
			{
				return false;
			}
			break;
		}
		default:
			return false;
		}

		regs.PC += opcode_size;

		return true;
	}

	bool execute_sub(z80::opcode op, z80::cpu_registers& regs)
	{
		size_t opcode_size = 1;

		switch (op)
		{
		case 0x90:
		case 0x91:
		case 0x92:
		case 0x93:
		case 0x94:
		case 0x95:
		case 0x97:
			sub(regs, r(op & 0b111, regs));
			break;
		case 0xD6:
			opcode_size = 2;
			sub(regs, fetch_pc<std::uint8_t>(+1));
			break;
		case 0x96:
			sub(regs, fetch_hl<std::uint8_t>());
			break;
		case 0xDD:
		case 0xFD:
		{
			if(fetch_pc(+1) == 0x96)
			{
				opcode_size = 3;
				auto d = fetch_pc<std::int8_t>(+2);
				auto base = idx(op, regs);

				sub(regs, fetch<std::uint8_t>(base + d));
			}
			else
			{
				return false;
			}
			break;
		}
		default:
			return false;
		}

		regs.PC += opcode_size;

		return true;
	}

	bool execute_sbc(z80::opcode op, z80::cpu_registers& regs)
	{
		size_t opcode_size = 1;

		switch (op)
		{
		case 0x98:
		case 0x99:
		case 0x9A:
		case 0x9B:
		case 0x9C:
		case 0x9D:
		case 0x9F:
			sbc(regs, r(op & 0b111, regs));
			break;
		case 0xDE:
			opcode_size = 2;
			sbc(regs, fetch_pc<std::uint8_t>(+1));
			break;
		case 0x9E:
			sbc(regs, fetch_hl<std::uint8_t>());
			break;
		case 0xDD:
		case 0xFD:
		{
			if(fetch_pc(+1) == 0x9E)
			{
				opcode_size = 3;
				auto d = fetch_pc<std::int8_t>(+2);
				auto base = idx(op, regs);

				sbc(regs, fetch<std::uint8_t>(base + d));
			}
			else
			{
				return false;
			}
			break;
		}
		default:
			return false;
		}

		regs.PC += opcode_size;

		return true;
	}

	bool execute_and_8(z80::opcode op, z80::cpu_registers& regs)
	{
		size_t opcode_size = 1;

		switch (op)
		{
		case 0xA0:
		case 0xA1:
		case 0xA2:
		case 0xA3:
		case 0xA4:
		case 0xA5:
		case 0xA7:
			and_8(regs, r(op & 0b111, regs));
			break;
		case 0xE6:
			opcode_size = 2;
			and_8(regs, fetch_pc<std::uint8_t>(+1));
			break;
		case 0xA6:
			and_8(regs, fetch_hl<std::uint8_t>());
			break;
		case 0xDD:
		case 0xFD:
		{
			if(fetch_pc(+1) == 0xA6)
			{
				opcode_size = 3;
				auto d = fetch_pc<std::int8_t>(+2);
				auto base = idx(op, regs);

				and_8(regs, fetch<std::uint8_t>(base + d));
			}
			else
			{
				return false;
			}
			break;
		}
		default:
			return false;
		}

		regs.PC += opcode_size;

		return true;
	}

protected:
	/* 8-Bit Arithmetic Group */
	// TODO: split instruction execution and decoding
	inline static void add(z80::cpu_registers& regs, std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;

		auto& flags = regs.main_set.flags;

		// check Half Carry Flag (H)
		flags.H = ((_a & 0x0f) + (_b & 0x0f) > 0xf) ? 1 : 0;

		// check Parity/Overflow Flag (P/V)
		flags.PV = (int)_a + (int)_b > 127 || (int)_a + (int)_b < -128 ? 1 : 0;

		// check Carry Flag (C)
		flags.C = (unsigned)_a + (unsigned)_b > 0xff ? 1 : 0;

		flags.N = 0;

		regs.main_set.A = _a + _b;

		flags.S = (regs.main_set.A < 0) ? 1 : 0;
		flags.Z = (regs.main_set.A == 0) ? 1 : 0;
	}

	inline static void adc(z80::cpu_registers& regs, std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;
		std::uint8_t _c = regs.main_set.flags.C;

		auto& flags = regs.main_set.flags;

		// check Half Carry Flag (H)
		flags.H = ((_a & 0x0f) + (_b & 0x0f) + _c > 0xf) ? 1 : 0;

		// check Parity/Overflow Flag (P/V)
		flags.PV = (int)_a + (int)_b + _c > 127 || (int)_a + (int)_b + _c < -128 ? 1 : 0;

		// check Carry Flag (C)
		flags.C = (unsigned)_a + (unsigned)_b + (unsigned)_c > 0xff ? 1 : 0;

		flags.N = 0;

		regs.main_set.A = _a + _b + _c;

		flags.S = (regs.main_set.A < 0) ? 1 : 0;
		flags.Z = (regs.main_set.A == 0) ? 1 : 0;
	}

	inline static void sub(z80::cpu_registers& regs, std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;

		auto& flags = regs.main_set.flags;

		// check Half Carry Flag (H)
		flags.H = (_b & 0x0f) > (_a & 0x0f) ? 1 : 0;

		// check Parity/Overflow Flag (P/V)
		flags.PV = (int)_a - (int)_b > 127 || (int)_a - (int)_b < -128 ? 1 : 0;

		// check Carry Flag (C)
		flags.C = _b > _a  ? 1 : 0;

		flags.N = 1;

		regs.main_set.A = _a - _b;

		flags.S = (regs.main_set.A < 0) ? 1 : 0;
		flags.Z = (regs.main_set.A == 0) ? 1 : 0;
	}

	inline static void sbc(z80::cpu_registers& regs, std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;
		std::uint8_t _c = regs.main_set.flags.C;

		auto& flags = regs.main_set.flags;

		// check Half Carry Flag (H)
		flags.H = (_b & 0x0f) + _c > (_a & 0x0f) ? 1 : 0;

		// check Parity/Overflow Flag (P/V)
		flags.PV = (int)_a - (int)_b - _c > 127 || (int)_a - (int)_b - _c < -128 ? 1 : 0;

		// check Carry Flag (C)
		flags.C = _b + _c > _a  ? 1 : 0;

		flags.N = 1;

		regs.main_set.A = _a - _b - _c;

		flags.S = (regs.main_set.A < 0) ? 1 : 0;
		flags.Z = (regs.main_set.A == 0) ? 1 : 0;
	}

	inline static void and_8(z80::cpu_registers& regs, std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;

		auto& flags = regs.main_set.flags;

		flags.H = flags.N = flags.C = 0;

		regs.main_set.A = _a & _b;

		// check Parity/Overflow Flag (P/V)
		flags.PV = check_parity(regs.main_set.A);

		flags.S = (regs.main_set.A < 0) ? 1 : 0;
		flags.Z = (regs.main_set.A == 0) ? 1 : 0;
	}
private:

	// decode helpers
	inline static std::uint16_t idx(z80::opcode op, z80::cpu_registers& regs)
	{
		assert(op == 0xDD || op == 0xFD);
		return op == 0xDD ? regs.IX : regs.IY;
	}

	inline static std::int8_t& r(z80::opcode rop, z80::cpu_registers& regs)
	{
		switch (rop)
		{
		case 0b111:
			return regs.main_set.A;
		case 0b000:
			return regs.main_set.B;
		case 0b001:
			return regs.main_set.C;
		case 0b010:
			return regs.main_set.D;
		case 0b011:
			return regs.main_set.E;
		case 0b100:
			return regs.main_set.H;
		case 0b101:
			return regs.main_set.L;
		}

		throw std::runtime_error("internal error: unknown register r");
	}
};


} // namespace genesis::z80
