#ifndef __OPERATIONS_HPP__
#define __OPERATIONS_HPP__

#include "z80/cpu.h"

#include <cstdint>

#include <iostream>
#include "string_utils.hpp"

namespace genesis::z80
{

class operations
{
public:
	operations(z80::cpu& cpu) : mem(cpu.memory()), regs(cpu.registers()), cpu(cpu)
	{
	}

	/* 8/16-Bit Load Group */
	template<class T>
	inline void ld_reg(T src, T& dest)
	{
		dest = src;
	}

	template<class T>
	inline void ld_at(T src, z80::memory::address dest_addr)
	{
		mem.write(dest_addr, src);
	}

	inline void ld_ir(std::int8_t src)
	{
		regs.main_set.A = src;

		auto& flags = regs.main_set.flags;

		flags.S = src < 0 ? 1 : 0;
		flags.Z = src == 0 ? 1 : 0;

		flags.H = flags.N = 0;

		// TODO: P/V should contain contents of IFF2
		flags.PV = 0;
	}

	inline void push(std::int16_t src)
	{
		regs.SP -= sizeof(src);
		mem.write(regs.SP, src);
	}

	inline void pop(std::int16_t& dest)
	{
		dest = mem.read<std::int16_t>(regs.SP);
		regs.SP += sizeof(dest);
	}

	/* Call and Return Group */
	inline void call(z80::memory::address addr)
	{
		regs.PC += 3; // always assume call instruction is 3 byte long
		push(regs.PC);
		regs.PC = addr;
	}

	inline bool check_cc(std::uint8_t cc)
	{
		auto& flags = regs.main_set.flags;
		switch(cc)
		{
		case 0b000:
			return flags.Z == 0;
		case 0b001:
			return flags.Z != 0;
		case 0b010:
			return flags.C == 0;
		case 0b011:
			return flags.C != 0;
		case 0b100:
			return flags.PV != 0;
		case 0b101:
			return flags.PV == 0;
		case 0b110:
			return flags.S == 0;
		case 0b111:
			return flags.S != 0;
		default:
			throw std::runtime_error("internal error: unsupported cc" + std::to_string(cc));
		}
	}

	inline void call_cc(std::uint8_t cc, z80::memory::address addr)
	{
		regs.PC += 3; // always assume call instruction is 3 byte long
		if(check_cc(cc))
		{
			push(regs.PC);
			regs.PC = addr;
		}
	}

	inline void ret()
	{
		pop((std::int16_t&)regs.PC);
	}

	/* Jump Group */
	inline void jp(z80::memory::address addr)
	{
		regs.PC = addr;
	}

	inline void jr_z(std::int8_t offset)
	{
		if(regs.main_set.flags.Z)
		{
			regs.PC += offset + 2;
		}
		else
		{
			regs.PC += 2;
		}
	}

	inline void jr(std::int8_t offset)
	{
		regs.PC += offset + 2;
	}

	/* CPU Control Groups */
	inline void di()
	{
		// TODO
	}

	inline void ei()
	{
		// TODO
	}

	/* Input and Output Group */
	inline void out(std::uint8_t n)
	{
		cpu.io_ports().out(n, regs.main_set.A);
	}

	/* 8-Bit Arithmetic Group */
	inline void add(std::int8_t b)
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

	inline void adc(std::int8_t b)
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

	inline void sub(std::int8_t b)
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

	inline void sbc(std::int8_t b)
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

	inline void and_8(std::int8_t b)
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

	inline void or_8(std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;

		auto& flags = regs.main_set.flags;

		flags.H = flags.N = flags.C = 0;

		regs.main_set.A = _a | _b;

		// check Parity/Overflow Flag (P/V)
		flags.PV = check_parity(regs.main_set.A);

		flags.S = (regs.main_set.A < 0) ? 1 : 0;
		flags.Z = (regs.main_set.A == 0) ? 1 : 0;
	}

	inline void xor_8(std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;

		auto& flags = regs.main_set.flags;

		flags.H = flags.N = flags.C = 0;

		regs.main_set.A = _a ^ _b;

		// check Parity/Overflow Flag (P/V)
		flags.PV = check_parity(regs.main_set.A);

		flags.S = (regs.main_set.A < 0) ? 1 : 0;
		flags.Z = (regs.main_set.A == 0) ? 1 : 0;
	}

	inline void cp(std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;

		auto& flags = regs.main_set.flags;

		flags.H = (_b & 0x0f) > (_a & 0x0f) ? 1 : 0;

		flags.PV = (int)_a - (int)_b > 127 || (int)_a - (int)_b < -128 ? 1 : 0;

		flags.S = (regs.main_set.A - b < 0) ? 1 : 0;

		flags.N = 1;
		flags.C = _b > _a  ? 1 : 0;
		flags.Z = (regs.main_set.A == b) ? 1 : 0;
	}

	inline void inc_reg(std::int8_t& r)
	{
		auto& flags = regs.main_set.flags;

		flags.N = 0;
		flags.PV = r == 127 ? 1 : 0;
		flags.H = (r & 0x0f) == 0x0f ? 1 : 0;
		
		r = ((std::uint8_t) r) + 1;

		flags.Z = r == 0 ? 1 : 0;
		flags.S = r < 0 ? 1 : 0;
	}

	inline void inc_at(z80::memory::address addr)
	{
		std::uint8_t val = mem.read<std::uint8_t>(addr);

		auto& flags = regs.main_set.flags;

		flags.N = 0;
		flags.PV = val == 127 ? 1 : 0;
		flags.H = (val & 0x0f) == 0x0f ? 1 : 0;
		
		++val;

		flags.Z = val == 0 ? 1 : 0;
		flags.S = (std::int8_t)val < 0 ? 1 : 0;

		mem.write(addr, val);
	}

	inline void dec_reg(std::int8_t& r)
	{
		auto& flags = regs.main_set.flags;

		flags.N = 0;
		flags.PV = r == -128 ? 1 : 0;
		flags.H = (r & 0x0f) == 0 ? 1 : 0;
		
		--r;

		flags.Z = r == 0 ? 1 : 0;
		flags.S = r < 0 ? 1 : 0;
	}

	inline void dec_at(z80::memory::address addr)
	{
		auto val = mem.read<std::int8_t>(addr);

		auto& flags = regs.main_set.flags;

		flags.N = 0;
		flags.PV = val == -128 ? 1 : 0;
		flags.H = (val & 0x0f) == 0 ? 1 : 0;
		
		--val;

		flags.Z = val == 0 ? 1 : 0;
		flags.S = val < 0 ? 1 : 0;

		mem.write(addr, val);
	}

	/* 16-Bit Arithmetic Group */
	inline void inc_reg_16(std::int16_t& reg)
	{
		// TODO: uint or int?
		++reg;
	}

private:
	/* utils */
	std::uint8_t static check_parity(int n)
	{
		int b;
		b = n ^ (n >> 1); 
		b = b ^ (b >> 2); 
		b = b ^ (b >> 4); 
		b = b ^ (b >> 8); 
		b = b ^ (b >> 16); 
		if (b & 1)
			return 1;
		else
			return 0;
	}

private:
	z80::memory& mem;
	z80::cpu_registers& regs;
	z80::cpu& cpu;
};

} // genesis namespace

#endif // __OPERATIONS_HPP__
