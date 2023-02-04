#ifndef __OPERATIONS_HPP__
#define __OPERATIONS_HPP__

#include "z80/cpu.h"

#include <optional>
#include <cstdint>
#include <limits>
#include <bit>

#include <iostream>
#include "string_utils.hpp"

namespace genesis::z80
{

using optional_reg_ref = std::optional<std::reference_wrapper<std::int8_t>>;


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

		std::int8_t diff = (std::uint8_t)regs.main_set.A - (std::uint8_t)b;
		set_sz_flags(diff);
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

	inline void ldi()
	{
		auto data = mem.read<std::int8_t>(regs.main_set.HL);
		mem.write(regs.main_set.DE, data);

		auto& flags = regs.main_set.flags;
		flags.H = flags.N = 0;
		flags.PV = regs.main_set.BC != 1 ? 1 : 0;

		inc_reg_16(regs.main_set.HL);
		inc_reg_16(regs.main_set.DE);
		dec_reg_16(regs.main_set.BC);
	}

	inline void ldir()
	{
		ldi();
		if(regs.main_set.BC == 0)
		{
			// instruction is terminated
			regs.PC += 2;
		}
		else
		{
			// do not change PC - repeat instruction
		}
	}

	inline void cpd()
	{
		auto data = mem.read<std::uint8_t>(regs.main_set.HL);
		std::uint8_t c = regs.main_set.flags.C;
		cp(data);

		regs.main_set.flags.PV = regs.main_set.BC != 1 ? 1 : 0;
		regs.main_set.flags.C = c;

		dec_reg_16(regs.main_set.HL);
		dec_reg_16(regs.main_set.BC);
	}

	inline void cpdr()
	{
		cpd();
		if(regs.main_set.BC == 0 || regs.main_set.flags.Z == 1)
		{
			// instruction is terminated
			regs.PC += 2;
		}
		else
		{
			// do not change PC - repeat instruction
		}
	}

	inline void cpi()
	{
		auto data = mem.read<std::int8_t>(regs.main_set.HL);
		std::uint8_t c = regs.main_set.flags.C;
		cp(data);

		regs.main_set.flags.PV = regs.main_set.BC != 1 ? 1 : 0;
		regs.main_set.flags.C = c;

		inc_reg_16(regs.main_set.HL);
		dec_reg_16(regs.main_set.BC);
	}

	inline void cpir()
	{
		cpi();
		if(regs.main_set.BC == 0 || regs.main_set.flags.Z == 1)
		{
			// instruction is terminated
			regs.PC += 2;
		}
		else
		{
			// do not change PC - repeat instruction
		}
	}

	inline void ldd()
	{
		auto data = mem.read<std::int8_t>(regs.main_set.HL);
		mem.write(regs.main_set.DE, data);

		auto& flags = regs.main_set.flags;
		flags.H = flags.N = 0;
		flags.PV = regs.main_set.BC != 1 ? 1 : 0;

		dec_reg_16(regs.main_set.HL);
		dec_reg_16(regs.main_set.DE);
		dec_reg_16(regs.main_set.BC);
	}

	inline void lddr()
	{
		ldd();
		if(regs.main_set.BC == 0)
		{
			// instruction is terminated
			regs.PC += 2;
		}
		else
		{
			// do not change PC - repeat instruction
		}
	}

	/* Rotate and Shift Group */
	inline void rlca()
	{
		std::uint8_t val = regs.main_set.A;
		regs.main_set.A = std::rotl(val, 1);

		regs.main_set.flags.H = regs.main_set.flags.N = 0;
		regs.main_set.flags.C = (val & 0b10000000) >> 7;
	}

	inline void rrca()
	{
		std::uint8_t val = regs.main_set.A;
		regs.main_set.A = std::rotr(val, 1);

		regs.main_set.flags.H = regs.main_set.flags.N = 0;
		regs.main_set.flags.C = val & 1;
	}

	inline void rla()
	{
		std::uint8_t val = regs.main_set.A;
		regs.main_set.A = (val << 1) | regs.main_set.flags.C;

		regs.main_set.flags.H = regs.main_set.flags.N = 0;
		regs.main_set.flags.C = (val & 0b10000000) >> 7;
	}

	inline void rra()
	{
		std::uint8_t val = regs.main_set.A;
		regs.main_set.A = (val >> 1) | (regs.main_set.flags.C << 7);

		regs.main_set.flags.H = regs.main_set.flags.N = 0;
		regs.main_set.flags.C = val & 1;
	}

	inline void rld()
	{
		auto data = mem.read<std::uint8_t>(regs.main_set.HL);
		std::uint8_t a = regs.main_set.A;

		regs.main_set.A = (a & 0xF0) | (data >> 4); // copy 4 high bits of HL -> 4 low bits of A
		data = data << 4; // copy 4 low bits of HL -> 4 high bits of HL
		data = (a & 0xF) | data; // copy 4 low bits of A -> 4 low bits of HL

		mem.write(regs.main_set.HL, data);

		auto& flags = regs.main_set.flags;
		flags.H = flags.N = 0;
		flags.PV = check_parity(regs.main_set.A);

		set_sz_flags(regs.main_set.A);
	}

	inline void rrd()
	{
		auto data = mem.read<std::uint8_t>(regs.main_set.HL);
		std::uint8_t a = regs.main_set.A;

		regs.main_set.A = (a & 0xF0) | (data & 0xF); // copy 4 low bits of HL -> 4 low bits of A
		data = data >> 4; // copy 4 high bits of HL -> 4 low bits of HL
		data |= (a & 0xF) << 4; // copy 4 low bits of A -> 4 high bits of HL

		mem.write(regs.main_set.HL, data);

		auto& flags = regs.main_set.flags;
		flags.H = flags.N = 0;
		flags.PV = check_parity(regs.main_set.A);

		set_sz_flags(regs.main_set.A);
	}

	/* Naming pattern
	 	r - rotate
		l/r - left/right
		(c) - circular
	*/

	inline void rlc(std::int8_t& src)
	{
		std::uint8_t val = src;
		src = std::rotl(val, 1);

		regs.main_set.flags.C = (val & 0b10000000) >> 7;
		set_rotate_shift_flags(src);
	}

	inline void rlc_at(z80::memory::address addr, optional_reg_ref dest = std::nullopt)
	{
		auto data = mem.read<std::int8_t>(addr);
		rlc(data);
		mem.write(addr, data);

		if(dest)
			dest->get() = data;
	}

	inline void rl(std::int8_t& src)
	{
		std::uint8_t val = src;
		src = (val << 1) | regs.main_set.flags.C;

		regs.main_set.flags.C = (val & 0b10000000) >> 7;
		set_rotate_shift_flags(src);
	}

	inline void rl_at(z80::memory::address addr, optional_reg_ref dest = std::nullopt)
	{
		auto data = mem.read<std::int8_t>(addr);
		rl(data);
		mem.write(addr, data);

		if(dest)
			dest->get() = data;
	}

	inline void rrc(std::int8_t& src)
	{
		std::uint8_t val = src;
		src = std::rotr(val, 1);

		regs.main_set.flags.C = val & 1;
		set_rotate_shift_flags(src);
	}

	inline void rrc_at(z80::memory::address addr, optional_reg_ref dest = std::nullopt)
	{
		auto data = mem.read<std::int8_t>(addr);
		rrc(data);
		mem.write(addr, data);

		if(dest)
			dest->get() = data;
	}

	inline void rr(std::int8_t& src)
	{
		std::uint8_t val = src;
		src = (val >> 1) | (regs.main_set.flags.C << 7);

		regs.main_set.flags.C = val & 1;
		set_rotate_shift_flags(src);
	}

	inline void rr_at(z80::memory::address addr, optional_reg_ref dest = std::nullopt)
	{
		auto data = mem.read<std::int8_t>(addr);
		rr(data);
		mem.write(addr, data);

		if(dest)
			dest->get() = data;
	}

	/* Naming pattern
	 	s - shift
		l/r - left/right
		a/l - arithmetical/logical
	*/

	inline void sla(std::int8_t& src)
	{
		std::uint8_t val = src;
		src = src << 1;

		regs.main_set.flags.C = (val & 0b10000000) >> 7;
		set_rotate_shift_flags(src);
	}

	inline void sla_at(z80::memory::address addr, optional_reg_ref dest = std::nullopt)
	{
		auto data = mem.read<std::int8_t>(addr);
		sla(data);
		mem.write(addr, data);

		if(dest)
			dest->get() = data;
	}

	inline void sra(std::int8_t& src)
	{
		std::uint8_t val = src;
		src = src >> 1;

		regs.main_set.flags.C = val & 1;
		set_rotate_shift_flags(src);
	}

	inline void sra_at(z80::memory::address addr, optional_reg_ref dest = std::nullopt)
	{
		auto data = mem.read<std::int8_t>(addr);
		sra(data);
		mem.write(addr, data);

		if(dest)
			dest->get() = data;
	}

	inline void srl(std::int8_t& src)
	{
		std::uint8_t val = src;
		src = val >> 1;

		regs.main_set.flags.C = val & 1;
		set_rotate_shift_flags(src);
	}

	inline void srl_at(z80::memory::address addr, optional_reg_ref dest = std::nullopt)
	{
		auto data = mem.read<std::int8_t>(addr);
		srl(data);
		mem.write(addr, data);

		if(dest)
			dest->get() = data;
	}

	inline void sll(std::int8_t& src)
	{
		std::uint8_t val = src;
		src = (src << 1) | 1;

		regs.main_set.flags.C = (val & 0b10000000) >> 7;
		set_rotate_shift_flags(src);
	}

	inline void sll_at(z80::memory::address addr, optional_reg_ref dest = std::nullopt)
	{
		auto data = mem.read<std::int8_t>(addr);
		sll(data);
		mem.write(addr, data);

		if(dest)
			dest->get() = data;
	}

	/* Bit Set, Reset, and Test Group */
	inline void tst_bit(std::uint8_t src, std::uint8_t bit)
	{
		bool is_set = ((src >> bit) & 1) != 0;

		auto& flags = regs.main_set.flags;

		flags.Z = is_set ? 0 : 1;
		flags.H = 1;
		flags.N = 0;
	}

	inline void set_bit(std::int8_t& dest, std::uint8_t bit)
	{
		dest |= (1 << bit);
	}

	inline void set_bit_at(z80::memory::address addr, std::uint8_t bit, optional_reg_ref dest = std::nullopt)
	{
		auto data = mem.read<std::uint8_t>(addr);
		data |= (1 << bit);
		mem.write(addr, data);

		if(dest)
			dest->get() = data;
	}

	inline void res_bit(std::int8_t& dest, std::uint8_t bit)
	{
		dest &= ~(1 << bit);
	}

	inline void res_bit_at(z80::memory::address addr, std::uint8_t bit, optional_reg_ref dest = std::nullopt)
	{
		auto data = mem.read<std::uint8_t>(addr);
		data &= ~(1 << bit);
		mem.write(addr, data);

		if(dest)
			dest->get() = data;
	}

	/* General-Purpose Arithmetic */
	inline void daa()
	{
		std::uint8_t val = regs.main_set.A;
		std::uint8_t corr = 0x0;

		if((val & 0xF) > 9 || regs.main_set.flags.H == 1)
		{
			corr = 0x06;
		}

		if((val >> 4) > 9 || regs.main_set.flags.C == 1 || ((val >> 4) >= 9 && (val & 0xF) > 9))
		{
			corr += 0x60;
			regs.main_set.flags.C = 1;
		}
		else
		{
			regs.main_set.flags.C = 0;
		}

		if(regs.main_set.flags.N)
		{
			regs.main_set.flags.H = check_half_borrow<std::uint8_t>(regs.main_set.A, corr);
			regs.main_set.A = val - corr;
		}
		else
		{
			regs.main_set.flags.H = check_half_carry<std::uint8_t>(regs.main_set.A, corr);
			regs.main_set.A = val + corr;
		}

		regs.main_set.flags.PV = check_parity(regs.main_set.A);
		set_sz_flags(regs.main_set.A);
	}

	inline void cpl()
	{
		regs.main_set.A = ~regs.main_set.A;
		regs.main_set.flags.H = regs.main_set.flags.N = 1;
	}

	inline void neg()
	{
		auto& flags = regs.main_set.flags;

		flags.C = check_borrow<std::uint8_t>(0, regs.main_set.A);
		flags.PV = check_overflow_sub<std::int8_t>(0, regs.main_set.A);
		flags.H = check_half_borrow<std::uint8_t>(0, regs.main_set.A);
		flags.N = 1;

		regs.main_set.A = (std::int8_t)0 - regs.main_set.A;

		set_sz_flags(regs.main_set.A);
	}

	inline void ccf()
	{
		auto& flags = regs.main_set.flags;
		flags.N = 0;
		flags.H = flags.C;
		flags.C = ~flags.C;
	}

	inline void scf()
	{
		auto& flags = regs.main_set.flags;
		flags.H = flags.N = 0;
		flags.C = 1;
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

		long sum = (long)(b & mask) + c;
		if(sum > (long)(a & mask))
			return 1;

		return 0;
	}

	template<class T>
	void set_sz_flags(T res)
	{
		static_assert(std::numeric_limits<T>::is_signed == true);

		regs.main_set.flags.S = res < 0 ? 1 : 0;
		regs.main_set.flags.Z = res == 0 ? 1 : 0;
	}

	void set_rotate_shift_flags(std::int8_t res)
	{
		set_sz_flags(res);
		regs.main_set.flags.H = regs.main_set.flags.N = 0;
		regs.main_set.flags.PV = check_parity(res);
	}

private:
	z80::memory& mem;
	z80::cpu_registers& regs;
	z80::cpu& cpu;
};

} // genesis namespace

#endif // __OPERATIONS_HPP__
