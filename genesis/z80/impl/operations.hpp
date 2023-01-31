#ifndef __OPERATIONS_HPP__
#define __OPERATIONS_HPP__

#include "z80/cpu.h"

#include <cstdint>
#include <limits>
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
	}

	inline void ret_cc(std::uint8_t cc)
	{
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

		flags.H = check_half_carry<std::uint8_t>(regs.main_set.A, b);
		flags.PV = check_overflow_add<std::int8_t>(regs.main_set.A, b);
		flags.C = check_carry<std::uint8_t>(regs.main_set.A, b);

		flags.N = 0;

		regs.main_set.A = _a + _b;

		set_sz_flags(regs.main_set.A);
	}

	inline void adc(std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;
		std::uint8_t _c = regs.main_set.flags.C;

		auto& flags = regs.main_set.flags;

		flags.H = check_half_carry<std::uint8_t>(regs.main_set.A, b, _c);
		flags.PV = check_overflow_add<std::int8_t>(regs.main_set.A, b, _c);
		flags.C = check_carry<std::uint8_t>(regs.main_set.A, b, _c);

		flags.N = 0;

		regs.main_set.A = _a + _b + _c;

		set_sz_flags(regs.main_set.A);
	}

	inline void sub(std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;

		auto& flags = regs.main_set.flags;

		flags.H = check_half_borrow<std::uint8_t>(regs.main_set.A, b);
		flags.PV = check_overflow_sub<std::int8_t>(regs.main_set.A, b);
		flags.C = check_borrow<std::uint8_t>(regs.main_set.A, b);

		flags.N = 1;

		regs.main_set.A = _a - _b;

		set_sz_flags(regs.main_set.A);
	}

	inline void sbc(std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;
		std::uint8_t _c = regs.main_set.flags.C;

		auto& flags = regs.main_set.flags;

		flags.H = check_half_borrow<std::uint8_t>(regs.main_set.A, b, _c);
		flags.PV = check_overflow_sub<std::int8_t>(regs.main_set.A, b, _c);
		flags.C = check_borrow<std::uint8_t>(regs.main_set.A, b, _c);

		flags.N = 1;

		regs.main_set.A = _a - _b - _c;

		set_sz_flags(regs.main_set.A);
	}

	inline void and_8(std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;

		auto& flags = regs.main_set.flags;

		flags.N = flags.C = 0;
		flags.H = 1;
		// flags.H = ((_a & 0b1000) & (_b & 0b1000)) >> 3;

		regs.main_set.A = _a & _b;

		flags.PV = check_parity(regs.main_set.A);

		set_sz_flags(regs.main_set.A);
	}

	inline void or_8(std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;

		auto& flags = regs.main_set.flags;

		flags.N = flags.C = 0;
		flags.H = 0;
		// flags.H = ((_a & 0b1000) | (_b & 0b1000)) >> 3;

		regs.main_set.A = _a | _b;

		flags.PV = check_parity(regs.main_set.A);

		set_sz_flags(regs.main_set.A);
	}

	inline void xor_8(std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;

		auto& flags = regs.main_set.flags;

		flags.H = flags.N = flags.C = 0;

		regs.main_set.A = _a ^ _b;

		flags.PV = check_parity(regs.main_set.A);

		set_sz_flags(regs.main_set.A);
	}

	inline void cp(std::int8_t b)
	{
		auto& flags = regs.main_set.flags;

		flags.H = check_half_borrow<std::uint8_t>(regs.main_set.A, b);
		flags.PV = check_overflow_sub<std::int8_t>(regs.main_set.A, b);
		flags.C = check_borrow<std::uint8_t>(regs.main_set.A, b);
		flags.N = 1;

		set_sz_flags(regs.main_set.A - b);
	}

	inline void inc_reg(std::int8_t& r)
	{
		auto& flags = regs.main_set.flags;

		std::uint8_t _r = r;

		flags.N = 0;
		flags.PV = _r == 127 ? 1 : 0;
		flags.H = (_r & 0xf) == 0xf ? 1 : 0;

		r = _r + 1;

		set_sz_flags(r);
	}

	inline void inc_at(z80::memory::address addr)
	{
		std::uint8_t val = mem.read<std::uint8_t>(addr);

		auto& flags = regs.main_set.flags;

		flags.N = 0;
		flags.PV = val == 127 ? 1 : 0;
		flags.H = (val & 0xf) == 0xf ? 1 : 0;
		
		++val;

		set_sz_flags((std::int8_t)val);

		mem.write(addr, val);
	}

	inline void dec_reg(std::int8_t& r)
	{
		auto& flags = regs.main_set.flags;

		std::uint8_t _r = r;

		flags.N = 1;
		flags.PV = r == -128 ? 1 : 0;
		flags.H = (_r & 0xf) == 0 ? 1 : 0;
	
		r = _r - 1;

		set_sz_flags(r);
	}

	inline void dec_at(z80::memory::address addr)
	{
		auto val = mem.read<std::uint8_t>(addr);

		auto& flags = regs.main_set.flags;

		flags.N = 1;
		flags.PV = (std::int8_t)val == -128 ? 1 : 0;
		flags.H = (val & 0xf) == 0 ? 1 : 0;
		
		--val;

		set_sz_flags((std::int8_t)val);

		mem.write(addr, val);
	}

	/* 16-Bit Arithmetic Group */
	inline void add_16(std::int16_t src, std::int16_t& dest)
	{
		std::uint16_t _dest = dest;
		std::uint16_t _src = src;

		auto& flags = regs.main_set.flags;

		flags.H = check_half_carry<std::uint16_t>(dest, src);
		flags.C = check_carry<std::uint16_t>(dest, src);

		flags.N = 0;

		dest = _dest + _src;

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

		flags.H = check_half_carry<std::uint16_t>(regs.main_set.HL, src, c);
		flags.C = check_carry<std::uint16_t>(regs.main_set.HL, src, c);
		flags.PV = check_overflow_add<std::int16_t>(regs.main_set.HL, src, c);

		flags.N = 0;

		regs.main_set.HL = _hl + _src + c;

		set_sz_flags(regs.main_set.HL);
	}

	inline void sbc_hl(std::int16_t src)
	{
		std::uint16_t _hl = regs.main_set.HL;
		std::uint16_t _src = src;
		std::uint16_t c = regs.main_set.flags.C;

		auto& flags = regs.main_set.flags;

		flags.H = check_half_borrow<std::uint16_t>(regs.main_set.HL, src, c);
		flags.C = check_borrow<std::uint16_t>(regs.main_set.HL, src, c);

		flags.PV = check_overflow_sub<std::int16_t>(regs.main_set.HL, src, c);

		flags.N = 1;

		regs.main_set.HL = _hl - _src - c;

		set_sz_flags(regs.main_set.HL);
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

	inline void neg()
	{
		auto& flags = regs.main_set.flags;
		std::uint8_t a = regs.main_set.A;

		// flags.C = a != 0 ? 1 : 0;
		// flags.PV = a == 0x80 ? 1 : 0;

		flags.C = check_borrow<std::uint8_t>(0, regs.main_set.A);
		flags.PV = check_overflow_sub<std::int8_t>(0, regs.main_set.A);
		flags.H = check_half_borrow<std::uint8_t>(0, regs.main_set.A);
		flags.N = 1;

		regs.main_set.A = (std::int8_t)0 - regs.main_set.A;

		set_sz_flags(regs.main_set.A);
	}

private:
	/* flag helpers */
	static std::uint8_t check_parity(std::uint8_t val)
	{
		int n = val;
		int b;
		b = n ^ (n >> 1); 
		b = b ^ (b >> 2); 
		b = b ^ (b >> 4); 
		b = b ^ (b >> 8); 
		b = b ^ (b >> 16); 
		if (b & 1)
			return 0;
		else
			return 1;
	}

	template<class T>
	static std::uint8_t check_overflow_add(T a, T b, std::uint8_t c = 0)
	{
		static_assert(std::numeric_limits<T>::is_signed == true);

		long sum = (long)a + (long)b + c;

		if(sum > std::numeric_limits<T>::max())
			return 1;
		
		if(sum < std::numeric_limits<T>::min())
			return 1;

		return 0;
	}

	template<class T>
	static std::uint8_t check_overflow_sub(T a, T b, std::uint8_t c = 0)
	{
		static_assert(std::numeric_limits<T>::is_signed == true);

		long diff = (long)a - (long)b - c;

		if(diff > std::numeric_limits<T>::max())
			return 1;

		if(diff < std::numeric_limits<T>::min())
			return 1;

		return 0;
	}

	template<class T>
	static std::uint8_t check_carry(T a, T b, std::uint8_t c = 0)
	{
		static_assert(std::numeric_limits<T>::is_signed == false);

		long sum = (long)a + (long)b + c;

		if(sum > std::numeric_limits<T>::max())
			return 1;

		return 0;
	}

	template<class T>
	static std::uint8_t check_borrow(T a, T b, std::uint8_t c = 0)
	{
		static_assert(std::numeric_limits<T>::is_signed == false);

		long sum = (long)b + c;

		if(sum > a)
			return 1;

		return 0;
	}

	template<class T>
	static std::uint8_t check_half_carry(T a, T b, std::uint8_t c = 0)
	{
		static_assert(std::numeric_limits<T>::is_signed == false);
		static_assert(sizeof(T) <= 2);

		const T mask = sizeof(T) == 1 ? 0xF : 0xFFF;
		long sum = (long)(a & mask) + (long)(b & mask) + c;

		if(sum > mask)
			return 1;

		return 0;
	}

	template<class T>
	static std::uint8_t check_half_borrow(T a, T b, std::uint8_t c = 0)
	{
		static_assert(std::numeric_limits<T>::is_signed == false);
		static_assert(sizeof(T) <= 2);

		const T mask = sizeof(T) == 1 ? 0xF : 0xFFF;

		auto a1 = [mask](T a, T b, std::uint8_t c) -> std::uint8_t
		{
			// if and only if the upper nibble had to change as a result of the operation on the lower nibble.
			T res = (T)(a & mask) - (T)(b & mask) - c;
			T upper = res & (~mask);
			if(upper != 0)
				return 1;
			return 0;
		};


		auto a2 = [mask](T a, T b, std::uint8_t c) -> std::uint8_t
		{
			long sum = (long)(b & mask) + c;

			if(sum > (long)(a & mask))
				return 1;

			return 0;
		};

		if(a1(a, b, c) != a2(a, b, c))
		{
			std::cout << "It makes differene!!!" << std::endl;
		}


		return a2(a, b, c);
	}

	template<class T>
	void set_sz_flags(T res)
	{
		static_assert(std::numeric_limits<T>::is_signed == true);

		regs.main_set.flags.S = res < 0 ? 1 : 0;
		regs.main_set.flags.Z = res == 0 ? 1 : 0;
	}

private:
	z80::memory& mem;
	z80::cpu_registers& regs;
	z80::cpu& cpu;
};

} // genesis namespace

#endif // __OPERATIONS_HPP__
