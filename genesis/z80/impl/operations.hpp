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

		set_sz(src);
		flags.H = flags.N = 0;

		flags.PV = regs.IFF2;
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

	void ret()
	{
		pop((std::int16_t&)regs.PC);
	}

	void reti()
	{
		retn();
		// NOTE: current interrupt model does not support multiple/nested interrupts
		// so there is no one to notify
	}

	void retn()
	{
		ret();
		regs.IFF1 = regs.IFF2;
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

	void djnz(std::int8_t offset)
	{
		regs.main_set.B = (std::uint8_t)regs.main_set.B - 1;
		if(regs.main_set.B != 0)
		{
			jr(offset);
		}
		else
		{
			regs.PC += 2;
		}
	}

	/* CPU Control Groups */
	void halt()
	{
		cpu.bus().set(bus::HALT);
	}

	inline void di()
	{
		regs.IFF1 = regs.IFF2 = 0;
	}

	inline void ei()
	{
		regs.IFF1 = regs.IFF2 = 1;
	}

	void im0()
	{
		cpu.interrupt_mode(cpu_interrupt_mode::im0);
	}

	void im1()
	{
		cpu.interrupt_mode(cpu_interrupt_mode::im1);
	}

	void im2()
	{
		cpu.interrupt_mode(cpu_interrupt_mode::im2);
	}

	/* Input and Output Group */

	void in(std::uint8_t n)
	{
		regs.main_set.A = cpu.io_ports().in(n, regs.main_set.A);
	}

	void in_reg(std::int8_t& reg)
	{
		std::int8_t input = cpu.io_ports().in(regs.main_set.C, regs.main_set.B);
		reg = input;

		regs.main_set.flags.H = regs.main_set.flags.N = 0;
		set_parity(input);
		set_sz(input);
	}

	void in_c()
	{
		std::int8_t input = cpu.io_ports().in(regs.main_set.C, regs.main_set.B);

		regs.main_set.flags.H = regs.main_set.flags.N = 0;
		set_parity(input);
		set_sz(input);
	}

	void ini()
	{
		std::uint8_t input = cpu.io_ports().in(regs.main_set.C, regs.main_set.B);
		mem.write(regs.main_set.HL, input);

		regs.main_set.B = (std::uint8_t)regs.main_set.B - 1;
		inc_reg_16(regs.main_set.HL);

		regs.main_set.flags.N = 1;
		regs.main_set.flags.Z = regs.main_set.B == 0;
	}

	void inir()
	{
		ini();
		if(regs.main_set.B == 0)
		{
			// terminate instruction
			regs.PC += 2;
		}
		else
		{
			// do nothing - repeat again
		}
	}

	void ind()
	{
		std::uint8_t input = cpu.io_ports().in(regs.main_set.C, regs.main_set.B);
		mem.write(regs.main_set.HL, input);

		regs.main_set.B = (std::uint8_t)regs.main_set.B - 1;
		dec_reg_16(regs.main_set.HL);

		regs.main_set.flags.N = 1;
		regs.main_set.flags.Z = regs.main_set.B == 0;
	}

	void indr()
	{
		ind();
		if(regs.main_set.B == 0)
		{
			// terminate instruction
			regs.PC += 2;
		}
		else
		{
			// do nothing - repeat again
		}
	}

	void out(std::uint8_t n)
	{
		cpu.io_ports().out(n, regs.main_set.A, regs.main_set.A);
	}

	void out_reg(std::int8_t reg)
	{
		cpu.io_ports().out(regs.main_set.C, regs.main_set.B, reg);
	}

	void outi()
	{
		std::uint8_t data = mem.read<std::uint8_t>(regs.main_set.HL);
		regs.main_set.B = (std::uint8_t)regs.main_set.B - 1;
		cpu.io_ports().out(regs.main_set.C, regs.main_set.B, data);
		inc_reg_16(regs.main_set.HL);

		regs.main_set.flags.N = 1;
		regs.main_set.flags.Z = regs.main_set.B == 0;
	}

	void otir()
	{
		outi();
		regs.main_set.flags.Z = 1;
		if(regs.main_set.B == 0)
		{
			// terminate instruction
			regs.PC += 2;
		}
		else
		{
			// do nothing - repeat again
		}
	}

	void outd()
	{
		std::uint8_t data = mem.read<std::uint8_t>(regs.main_set.HL);
		regs.main_set.B = (std::uint8_t)regs.main_set.B - 1;
		cpu.io_ports().out(regs.main_set.C, regs.main_set.B, data);
		dec_reg_16(regs.main_set.HL);

		regs.main_set.flags.N = 1;
		regs.main_set.flags.Z = regs.main_set.B == 0;
	}

	void otdr()
	{
		outd();
		regs.main_set.flags.Z = 1;
		if(regs.main_set.B == 0)
		{
			// terminate instruction
			regs.PC += 2;
		}
		else
		{
			// do nothing - repeat again
		}
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

		set_sz(regs.main_set.A);
		set_yx(regs.main_set.A);
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

		set_sz(regs.main_set.A);
		set_yx(regs.main_set.A);
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

		set_sz(regs.main_set.A);
		set_yx(regs.main_set.A);
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

		set_sz(regs.main_set.A);
		set_yx(regs.main_set.A);
	}

	inline void and_8(std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;

		auto& flags = regs.main_set.flags;

		flags.N = flags.C = 0;
		flags.H = 1;

		regs.main_set.A = _a & _b;

		set_parity(regs.main_set.A);

		set_sz(regs.main_set.A);
		set_yx(regs.main_set.A);
	}

	inline void or_8(std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;

		auto& flags = regs.main_set.flags;

		flags.H = flags.N = flags.C = 0;

		regs.main_set.A = _a | _b;

		set_parity(regs.main_set.A);
		set_sz(regs.main_set.A);
		set_yx(regs.main_set.A);
	}

	inline void xor_8(std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;

		auto& flags = regs.main_set.flags;

		flags.H = flags.N = flags.C = 0;

		regs.main_set.A = _a ^ _b;

		set_parity(regs.main_set.A);
		set_sz(regs.main_set.A);
		set_yx(regs.main_set.A);
	}

	inline void cp(std::int8_t b)
	{
		auto& flags = regs.main_set.flags;

		flags.H = check_half_borrow<std::uint8_t>(regs.main_set.A, b);
		flags.PV = check_overflow_sub<std::int8_t>(regs.main_set.A, b);
		flags.C = check_borrow<std::uint8_t>(regs.main_set.A, b);
		flags.N = 1;

		std::int8_t diff = (std::uint8_t)regs.main_set.A - (std::uint8_t)b;
		set_sz(diff);
		set_yx(b);
	}

	inline void inc_reg(std::int8_t& r)
	{
		auto& flags = regs.main_set.flags;

		std::uint8_t _r = r;

		flags.N = 0;
		flags.PV = _r == 127 ? 1 : 0;
		flags.H = (_r & 0xf) == 0xf ? 1 : 0;

		r = _r + 1;

		set_sz(r);
		set_yx(r);
	}

	inline void inc_at(z80::memory::address addr)
	{
		auto val = mem.read<std::int8_t>(addr);
		inc_reg(val);
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

		set_sz(r);
		set_yx(r);
	}

	inline void dec_at(z80::memory::address addr)
	{
		auto val = mem.read<std::int8_t>(addr);
		dec_reg(val);
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
		set_yx(high);
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

		set_sz(regs.main_set.HL);
		set_yx(regs.main_set.H);
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

		set_sz(regs.main_set.HL);
		set_yx(regs.main_set.H);
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

	void ex_16_at(std::int16_t& reg, z80::memory::address addr)
	{
		auto data = mem.read<std::int16_t>(addr);
		mem.write(addr, reg);
		reg = data;
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

		set_yx_ld(data);
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
		set_yx_cp(data);
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
		set_yx_cp(data);
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

		set_yx_ld(data);
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
		set_yx(regs.main_set.A);
	}

	inline void rrca()
	{
		std::uint8_t val = regs.main_set.A;
		regs.main_set.A = std::rotr(val, 1);

		regs.main_set.flags.H = regs.main_set.flags.N = 0;
		regs.main_set.flags.C = val & 1;
		set_yx(regs.main_set.A);
	}

	inline void rla()
	{
		std::uint8_t val = regs.main_set.A;
		regs.main_set.A = (val << 1) | regs.main_set.flags.C;

		regs.main_set.flags.H = regs.main_set.flags.N = 0;
		regs.main_set.flags.C = (val & 0b10000000) >> 7;
		set_yx(regs.main_set.A);
	}

	inline void rra()
	{
		std::uint8_t val = regs.main_set.A;
		regs.main_set.A = (val >> 1) | (regs.main_set.flags.C << 7);

		regs.main_set.flags.H = regs.main_set.flags.N = 0;
		regs.main_set.flags.C = val & 1;
		set_yx(regs.main_set.A);
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

		set_parity(regs.main_set.A);
		set_sz(regs.main_set.A);
		set_yx(regs.main_set.A);
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

		set_parity(regs.main_set.A);
		set_sz(regs.main_set.A);
		set_yx(regs.main_set.A);
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

		flags.S = bit == 7 && is_set;
		flags.PV = flags.Z;
		set_yx(src);
	}

	inline void tst_bit_at(z80::memory::address addr, std::uint8_t bit)
	{
		auto data = mem.read<std::uint8_t>(addr);
		tst_bit(data, bit);
		set_yx(addr >> 8);
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
		std::uint8_t low = val & 0xF;
		std::uint8_t high = val >> 4;
		std::uint8_t corr = 0x0;

		if(low > 9 || regs.main_set.flags.H)
			corr = 0x06;

		if(high > 9 || regs.main_set.flags.C || (high >= 9 && low > 9))
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

		set_parity(regs.main_set.A);
		set_sz(regs.main_set.A);
		set_yx(regs.main_set.A);
	}

	inline void cpl()
	{
		regs.main_set.A = ~regs.main_set.A;
		regs.main_set.flags.H = regs.main_set.flags.N = 1;
		set_yx(regs.main_set.A);
	}

	inline void neg()
	{
		auto& flags = regs.main_set.flags;

		flags.C = check_borrow<std::uint8_t>(0, regs.main_set.A);
		flags.PV = check_overflow_sub<std::int8_t>(0, regs.main_set.A);
		flags.H = check_half_borrow<std::uint8_t>(0, regs.main_set.A);
		flags.N = 1;

		regs.main_set.A = (std::int8_t)0 - regs.main_set.A;

		set_sz(regs.main_set.A);
		set_yx(regs.main_set.A);
	}

	inline void ccf()
	{
		auto& flags = regs.main_set.flags;
		flags.N = 0;
		flags.H = flags.C;
		flags.C = ~flags.C;
		set_yx(regs.main_set.A);
	}

	inline void scf()
	{
		auto& flags = regs.main_set.flags;
		flags.H = flags.N = 0;
		flags.C = 1;
		set_yx(regs.main_set.A);
	}

	/* Interrupts */
	void nonmaskable_interrupt()
	{
		cpu.bus().clear(bus::HALT);
		regs.IFF1 = 0;
		push(regs.PC);
		regs.PC = 0x66;
	}

	void maskable_interrupt_m0()
	{
		cpu.bus().clear(bus::HALT);
		di();
		// someone else should also execute required instruction
	}

	void maskable_interrupt_m1()
	{
		cpu.bus().clear(bus::HALT);
		di();
		push(regs.PC);
		regs.PC = 0x38;
	}

	void maskable_interrupt_m2(std::uint8_t data)
	{
		cpu.bus().clear(bus::HALT);
		di();
		push(regs.PC);

		data = data & 0b11111110;

		// form a pointer to a vector table
		memory::address addr = (std::uint8_t)regs.I;
		addr = addr << 8;
		addr = addr | data;

		// read interrupt routine address
		regs.PC = mem.read<memory::address>(addr);
	}

private:
	/* flag helpers */
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
	void set_sz(T res)
	{
		static_assert(std::numeric_limits<T>::is_signed == true);

		regs.main_set.flags.S = res < 0 ? 1 : 0;
		regs.main_set.flags.Z = res == 0 ? 1 : 0;
	}

	void set_rotate_shift_flags(std::int8_t res)
	{
		set_sz(res);
		set_yx(res);
		set_parity(res);
		regs.main_set.flags.H = regs.main_set.flags.N = 0;
	}

	void set_parity(std::uint8_t res)
	{
		int n = res;
		int b;
		b = n ^ (n >> 1);
		b = b ^ (b >> 2);
		b = b ^ (b >> 4);
		b = b ^ (b >> 8);
		b = b ^ (b >> 16);

		regs.main_set.flags.PV = !(b & 1);
	}

	void set_yx(std::uint8_t res)
	{
		regs.main_set.flags.X = (res & 0b00001000) >> 3;
		regs.main_set.flags.Y = (res & 0b00100000) >> 5;
	}

	void set_yx_cp(std::uint8_t hl)
	{
		std::uint8_t n = (std::uint8_t)regs.main_set.A - hl - (std::uint8_t)regs.main_set.flags.H;
		regs.main_set.flags.X = (n & 0b00001000) >> 3;
		regs.main_set.flags.Y = (n & 0b00000010) >> 1;
	}

	void set_yx_ld(std::uint8_t hl)
	{
		std::uint8_t n = hl + (std::uint8_t)regs.main_set.A;
		regs.main_set.flags.X = (n & 0b00001000) >> 3;
		regs.main_set.flags.Y = (n & 0b00000010) >> 1;
	}
private:
	z80::memory& mem;
	z80::cpu_registers& regs;
	z80::cpu& cpu;
};

} // genesis namespace

#endif // __OPERATIONS_HPP__
