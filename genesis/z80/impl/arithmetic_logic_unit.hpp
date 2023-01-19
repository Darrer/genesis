#include "base_unit.hpp"

#include "string_utils.hpp"

#include <cstddef>
#include <iostream>
#include <cassert>


namespace genesis::z80
{

enum OP : z80::opcode
{
	ADD,
	ADC,
	SUB,
	SBC,
	AND
};

// register addresing 
// format: operation, r bit map, {op codes for registers}
struct register_addresing_table_record
{
	OP opcode;
	std::uint8_t r_mask;
	std::uint8_t opcode_mask;
};

std::initializer_list<register_addresing_table_record> register_addresing_ops = {
	{ OP::ADD, 0b111, 0b10000000 },
	{ OP::ADC, 0b111, 0b10001000 },
	{ OP::SUB, 0b111, 0b10010000 },
	{ OP::SBC, 0b111, 0b10011000 },
	{ OP::AND, 0b111, 0b10100000 },
};

class cpu::unit::arithmetic_logic_unit : public cpu::unit::base_unit
{
public:
	using base_unit::base_unit;

	bool execute() override
	{
		auto& regs = cpu.registers();

		z80::opcode op = fetch_pc();

		if(try_execute_register_addresing(op, regs))
			return true;

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
	bool try_execute_register_addresing(z80::opcode opcode, z80::cpu_registers& regs)
	{
		for(const auto& inst: register_addresing_ops)
		{
			z80::opcode masked_opcode = opcode & (~inst.r_mask);
			if(masked_opcode == inst.opcode_mask)
			{
				// found opcode
				// TODO: need way to determine it
				const std::uint8_t offset = 0;

				std::uint8_t r_code = (opcode & inst.r_mask);// >> offset;

				if(r_code == 0b110)
					return false;

				auto& b = r(r_code, regs);

				exec(inst.opcode, b);

				regs.PC += 1;

				return true;
			}
		}

		return false;
	}

	void exec(OP op, std::int8_t b)
	{
		auto& regs = cpu.registers();
		switch (op)
		{
		case OP::ADD:
			add(regs, b);
			break;
		case OP::ADC:
			adc(regs, b);
			break;
		case OP::SUB:
			sub(regs, b);
			break;
		case OP::SBC:
			sbc(regs, b);
			break;
		case OP::AND:
			and_8(regs, b);
			break;
		default:
			throw std::runtime_error("internal error, opcode must be found: try_execute_register_addresing");
			break;
		}
	}

private:
	bool execute_add(z80::opcode op, z80::cpu_registers& regs)
	{
		size_t opcode_size = 1;

		switch (op)
		{
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
	// TODO: also create Decoder static class - with methods like decode_register_addressing - will 
	// TODO: move to Exec/Impl/Operations/Executioner static class
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

		throw std::runtime_error("internal error: unknown register r (0b" + su::bin_str(rop) + ")");
	}
};


} // namespace genesis::z80
