#ifndef __OPERATIONS_HPP__
#define __OPERATIONS_HPP__

#include "z80/cpu.h"

#include <cstdint>
#include <iostream>
#include <bit>

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
		static_assert(sizeof(T) <= 2);
		dest = src;
	}

	inline void ld_reg_from(std::int16_t& dest, z80::memory::address addr)
	{
		dest = mem.read<std::int16_t>(addr);
	}

	template<class T>
	inline void ld_at(T src, z80::memory::address dest_addr)
	{
		static_assert(sizeof(T) <= 2);
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
		// std::cout << "call" << std::endl;
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
			return flags.Z == 1;
		case 0b010:
			return flags.C == 0;
		case 0b011:
			return flags.C == 1;
		case 0b100:
			return flags.PV == 0;
		case 0b101:
			return flags.PV == 1;
		case 0b110:
			return flags.S == 0;
		case 0b111:
			return flags.S == 1;
		default:
			throw std::runtime_error("check_cc internal error: unsupported cc" + std::to_string(cc));
		}
	}

	inline void call_cc(std::uint8_t cc, z80::memory::address addr)
	{
		// std::cout << "call_cc cc: " << su::bin_str(cc) << " " << su::hex_str(addr)  << "; " << std::boolalpha << check_cc(cc) << std::endl;
		regs.PC += 3; // always assume call instruction is 3 byte long
		if(check_cc(cc))
		{
			push(regs.PC);
			regs.PC = addr;
		}
	}

	inline void rst(std::uint8_t cc)
	{
		regs.PC += 1;
		push(regs.PC);
		regs.PC = cc * 0x8;
	}

	inline void ret()
	{
		pop((std::int16_t&)regs.PC);
		// std::cout << "ret to " << su::hex_str(regs.PC) << std::endl;
	}

	inline void ret_cc(std::uint8_t cc)
	{
		// std::cout << "ret_cc " << su::bin_str(cc) << "; " << std::boolalpha << check_cc(cc) << std::endl;
		if(check_cc(cc))
		{
			ret();
		}
		else
		{
			regs.PC += 1;
		}
	}

	/* Jump Group */
	inline void jp(z80::memory::address addr)
	{
		regs.PC = addr;
	}

	inline void jp_cc(std::uint8_t cc, z80::memory::address addr)
	{
		if(check_cc(cc))
		{
			jp(addr);
		}
		else
		{
			regs.PC += 3;
		}
	}

	inline void jr_z(std::int8_t offset)
	{
		if(regs.main_set.flags.Z)
		{
			jr(offset);
		}
		else
		{
			regs.PC += 2;
		}
	}

	inline void jr_c(std::int8_t offset)
	{
		if(regs.main_set.flags.C)
		{
			jr(offset);
		}
		else
		{
			regs.PC += 2;
		}
	}

	inline void jr_nz(std::int8_t offset)
	{
		if(regs.main_set.flags.Z == 0)
		{
			jr(offset);
		}
		else
		{
			regs.PC += 2;
		}
	}

	inline void jr_nc(std::int8_t offset)
	{
		if(regs.main_set.flags.C == 0)
		{
			jr(offset);
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

		flags.N = flags.C = 0;
		flags.H = 1;

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

		r++;

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

		flags.N = 1;
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

		flags.N = 1;
		flags.PV = val == -128 ? 1 : 0;
		flags.H = (val & 0x0f) == 0 ? 1 : 0;
		
		--val;

		flags.Z = val == 0 ? 1 : 0;
		flags.S = val < 0 ? 1 : 0;

		mem.write(addr, val);
	}

	/* 16-Bit Arithmetic Group */
	inline void add_16(std::int16_t src, std::int16_t& dest)
	{
		std::uint16_t _hl = dest;
		std::uint16_t _src = src;

		auto& flags = regs.main_set.flags;

		flags.H = ((_hl & 0xfff) + (_src & 0xfff) > 0xfff) ? 1 : 0;
		flags.C = (unsigned long)_hl + (unsigned long)_src > 0xfffful ? 1 : 0;

		flags.N = 0;

		dest = _hl + _src;

		std::uint8_t high = dest >> 8;
		flags.X2 = (high & 0b00100000) >> 5;
		flags.X1 = (high & 0b00001000) >> 3;
	}

	inline void adc_hl(std::int16_t src)
	{
		std::uint16_t _hl = regs.main_set.HL;
		std::uint16_t _src = src;
		std::uint16_t c = regs.main_set.flags.C;

		auto& flags = regs.main_set.flags;

		flags.H = ((_hl & 0xfff) + (_src & 0xfff) + c > 0xfff) ? 1 : 0;
		flags.C = (unsigned long)_hl + (unsigned long)_src + (unsigned long)c > 0xfffful ? 1 : 0;

		flags.PV = (long)regs.main_set.HL + (long)src + c > 32767
			|| (long)regs.main_set.HL + (long)src + c < -32768 ? 1 : 0;

		flags.N = 0;

		regs.main_set.HL = _hl + _src + c;

		flags.S = regs.main_set.HL < 0 ? 1 : 0;
		flags.Z = regs.main_set.HL == 0 ? 1 : 0;
	}

	inline void sbc_hl(std::int16_t src)
	{
		std::uint16_t _hl = regs.main_set.HL;
		std::uint16_t _src = src;
		std::uint16_t c = regs.main_set.flags.C;

		auto& flags = regs.main_set.flags;

		flags.H = ((_src & 0xfff) + c > (_hl & 0xfff)) ? 1 : 0;
		flags.C = ((long)_src + c > _hl) ? 1 : 0;
		flags.PV = (long)regs.main_set.HL - (long)src - c > 32767
			|| (long)regs.main_set.HL - (long)src - c < -32768 ? 1 : 0;

		flags.N = 1;

		regs.main_set.HL = _hl - _src - c;

		flags.S = regs.main_set.HL < 0 ? 1 : 0;
		flags.Z = regs.main_set.HL == 0 ? 1 : 0;
	}

	inline void inc_reg_16(std::int16_t& reg)
	{
		reg = (std::uint16_t)reg + 1;
	}

	inline void dec_reg_16(std::int16_t& reg)
	{
		reg = (std::uint16_t)reg - 1;
	}

	/* Exchange, Block Transfer, and Search Group */
	inline void ex_de_hl()
	{
		std::swap(regs.main_set.DE, regs.main_set.HL);
	}

	inline void ex_af_afs()
	{
		std::swap(regs.main_set.AF, regs.alt_set.AF);
	}

	inline void exx()
	{
		std::swap(regs.main_set.BC, regs.alt_set.BC);
		std::swap(regs.main_set.DE, regs.alt_set.DE);
		std::swap(regs.main_set.HL, regs.alt_set.HL);
	}

	inline void ldir()
	{
		regs.main_set.flags.H = regs.main_set.flags.N = 0;
		regs.main_set.flags.PV = 0; // TODO: not sure what to do with this flag

		// write (HL) -> (DE)
		std::uint8_t data = mem.read<std::uint8_t>(regs.main_set.HL);
		mem.write(regs.main_set.DE, data);

		// update registers
		regs.main_set.HL++;
		regs.main_set.DE++;
		regs.main_set.BC--;

		// check if need to repeat
		if(regs.main_set.BC == 0)
		{
			regs.PC += 2;
			return;
		}

		// do not change PC, repeat instruction instead
	}

	/* Rotate and Shift Group */
	inline void rlca()
	{
		std::uint8_t val = regs.main_set.A;
		regs.main_set.A = std::rotl(val, 1);

		regs.main_set.flags.H = regs.main_set.flags.N = 0;
		// regs.main_set.flags.C = val & 0b10000000;
		regs.main_set.flags.C = val >= 128  ? 1 : 0;
	}

	inline void rrca()
	{
		std::uint8_t val = regs.main_set.A;
		regs.main_set.A = std::rotr(val, 1);

		regs.main_set.flags.H = regs.main_set.flags.N = 0;
		regs.main_set.flags.C = val % 2 == 0 ? 0 : 1;
		// regs.main_set.flags.C = val & 1;
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
