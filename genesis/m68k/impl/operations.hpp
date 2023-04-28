#ifndef __M68K_OPERATIONS_HPP__
#define __M68K_OPERATIONS_HPP__

#include <bit>
#include <cstdint>
#include <cstdlib>

#include <iostream>

#include "cpu_flags.hpp"
#include "exception.hpp"

#include "instruction_type.h"
#include "m68k/cpu_registers.hpp"
#include "ea_decoder.hpp"


namespace genesis::m68k
{

class operations
{
public:
	operations() = delete;

	template<class T1, class T2>
	static std::uint32_t add(T1 a, T2 b, size_type size, status_register& sr)
	{
		return add(value(a, size), value(b, size), size, sr);
	}

	template<class T1, class T2>
	static std::uint32_t adda(T1 src, T2 dest, size_type size, status_register&)
	{
		if(size == size_type::WORD)
			return (std::int16_t)value(src, size) + value(dest, size_type::LONG);
		return value(src, size) + value(dest, size);
	}

	template<class T1, class T2>
	static std::uint32_t addx(T1 a, T2 b, size_type size, status_register& sr)
	{
		return addx(value(a, size), value(b, size), size, sr);
	}

	template<class T1, class T2>
	static std::uint32_t sub(T1 a, T2 b, size_type size, status_register& sr)
	{
		return sub(value(a, size), value(b, size), size, sr);
	}

	template<class T1, class T2>
	static std::uint32_t subx(T1 a, T2 b, size_type size, status_register& sr)
	{
		return subx(value(a, size), value(b, size), size, sr);
	}

	template<class T1, class T2>
	static std::uint32_t cmp(T1 a, T2 b, size_type size, status_register& sr)
	{
		// cmp like sub but does not change the result
		std::uint8_t old_x = sr.X;
		sub(value(a, size), value(b, size), size, sr);
		sr.X = old_x; // in case of CMP X is not affected
		return value(a, size);
	}

	template<class T1, class T2>
	static std::uint32_t cmpa(T1 src, T2 dest, size_type size, status_register& sr)
	{
		std::uint32_t b = value(dest, size_type::LONG);
		std::uint32_t a;
		if(size == size_type::WORD)
			a = (std::int32_t)(std::int16_t)value(src, size);
		else
			a = value(src, size);
		return cmp(b, a, size_type::LONG, sr);
	}

	template<class T1, class T2>
	static std::uint32_t suba(T1 src, T2 dest, size_type size, status_register&)
	{
		if(size == size_type::WORD)
			return value(dest, size_type::LONG) - (std::int16_t)value(src, size);
		return value(dest, size) - value(src, size);
	}

	template<class T1, class T2>
	static std::uint32_t and_op(T1 a, T2 b, size_type size, status_register& sr)
	{
		return and_op(value(a, size), value(b, size), size, sr);
	}

	static void andi_to_ccr(std::uint8_t src, std::uint16_t& SR)
	{
		std::uint16_t src_mask = 0xFFE0 | src;
		SR = SR & src_mask;
	}

	static void andi_to_sr(std::uint16_t src, std::uint16_t& SR)
	{
		src = clear_unimplemented_flags(src);
		SR = SR & src;
	}

	template<class T1, class T2>
	static std::uint32_t or_op(T1 a, T2 b, size_type size, status_register& sr)
	{
		return or_op(value(a, size), value(b, size), size, sr);
	}

	static void ori_to_sr(std::uint16_t src, std::uint16_t& SR)
	{
		src = clear_unimplemented_flags(src);
		SR = SR | src;
	}

	static void ori_to_ccr(std::uint8_t src, std::uint16_t& SR)
	{
		SR = SR | (src & 0b11111);
	}

	template<class T1, class T2>
	static std::uint32_t eor(T1 a, T2 b, size_type size, status_register& sr)
	{
		return eor(value(a, size), value(b, size), size, sr);
	}

	static void eori_to_sr(std::uint16_t src, std::uint16_t& SR)
	{
		src = clear_unimplemented_flags(src);
		SR = SR ^ src;
	}

	static void eori_to_ccr(std::uint8_t src, std::uint16_t& SR)
	{
		std::uint8_t res = (SR ^ src) & 0b11111;
		SR = SR & ~0b11111;
		SR = SR | res;
	}

	template<class T1>
	static std::uint32_t neg(T1 a, size_type size, status_register& sr)
	{
		return sub(0, value(a, size), size, sr);
	}

	template<class T1>
	static std::uint32_t negx(T1 a, size_type size, status_register& sr)
	{
		return subx(0, value(a, size), size, sr);
	}

	template<class T1>
	static std::uint32_t not_op(T1 a, size_type size, status_register& sr)
	{
		std::uint32_t res = value(~value(a, size), size);
		set_nz_flags(res, size, sr);
		sr.V = sr.C = 0;
		return res;
	}

	template<class T1>
	static std::uint32_t move(T1 src, size_type size, status_register& sr)
	{
		std::uint32_t res = value(src, size);
		set_nz_flags(res, size, sr);
		sr.V = sr.C = 0;
		return res;
	}

	template<class T1>
	static std::uint32_t movea(T1 src, size_type size)
	{
		if(size == size_type::LONG)
			return value(src, size);
		return sign_extend(value(src, size));
	}

	template<class T1>
	static std::uint16_t move_to_sr(T1 src)
	{
		std::uint16_t res = value(src, size_type::WORD);
		return clear_unimplemented_flags(res);
	}

	template<class T1>
	static std::uint16_t move_to_ccr(T1 src, std::uint16_t sr)
	{
		std::uint8_t low_byte = value(src, size_type::BYTE) & 0b11111;
		return (sr & 0xFF00) | low_byte;
	}

	template<class T1>
	static std::uint32_t asl(T1 a, std::uint32_t shift_count, size_type size, status_register& sr)
	{
		std::int32_t val = value(a, size);
		shift_count = shift_count % 64;

		sr.C = sr.V = 0;
		for(std::uint32_t i = 0; i < shift_count; ++i)
		{
			sr.C = sr.X = msb(val, size);
			val = val << 1;
			sr.V = sr.V | (sr.C ^ msb(val, size));
		}

		val = value(val, size);
		set_nz_flags(val, size, sr);

		return val;
	}

	template<class T1>
	static std::uint32_t asr(T1 a, std::uint32_t shift_count, size_type size, status_register& sr)
	{
		std::int32_t val = value(a, size);
		shift_count = shift_count % 64;

		sr.C = sr.V = 0;
		// const std::uint32_t set_flags_limit = size_raw(size) * CHAR_BIT;
		for(std::uint32_t i = 0; i < shift_count; ++i)
		{
			// if(i < set_flags_limit)
				sr.C = sr.X = lsb(val);
			// else
				// sr.C = sr.X = 0;

			if(size == size_type::BYTE)
				val = std::int8_t(val) >> 1;
			else if(size == size_type::WORD)
				val = std::int16_t(val) >> 1;
			else
				val = std::int32_t(val) >> 1;
		}

		val = value(val, size);
		set_nz_flags(val, size, sr);

		return val;
	}

	template<class T1>
	static std::uint32_t rol(T1 a, std::uint32_t shift_count, size_type size, status_register& sr)
	{
		std::uint32_t val = value(a, size);
		shift_count = shift_count % 64;

		if(size == size_type::BYTE)
			val = std::rotl(std::uint8_t(val), shift_count);
		else if(size == size_type::WORD)
			val = std::rotl(std::uint16_t(val), shift_count);
		else
			val = std::rotl(val, shift_count);

		sr.C = shift_count == 0 ? 0 : lsb(val);
		set_nz_flags(val, size, sr);
		sr.V = 0;
		return val;
	}

	template<class T1>
	static std::uint32_t ror(T1 a, std::uint32_t shift_count, size_type size, status_register& sr)
	{
		std::uint32_t val = value(a, size);
		shift_count = shift_count % 64;

		if(size == size_type::BYTE)
			val = std::rotr(std::uint8_t(val), shift_count);
		else if(size == size_type::WORD)
			val = std::rotr(std::uint16_t(val), shift_count);
		else
			val = std::rotr(val, shift_count);

		sr.C = shift_count == 0 ? 0 : msb(val, size);
		set_nz_flags(val, size, sr);
		sr.V = 0;
		return val;
	}

	template<class T1>
	static std::uint32_t roxl(T1 a, std::uint32_t shift_count, size_type size, status_register& sr)
	{
		std::uint32_t val = value(a, size);
		shift_count = shift_count % 64;

		sr.C = sr.X;
		for(std::uint32_t i = 0; i < shift_count; ++i)
		{
			sr.C = msb(val, size);

			val = val << 1;
			val = val | sr.X;

			sr.X = sr.C;
		}

		set_nz_flags(val, size, sr);
		sr.V = 0;
		return val;
	}

	template<class T1>
	static std::uint32_t roxr(T1 a, std::uint32_t shift_count, size_type size, status_register& sr)
	{
		std::uint32_t val = value(a, size);
		shift_count = shift_count % 64;

		sr.C = sr.X;
		for(std::uint32_t i = 0; i < shift_count; ++i)
		{
			sr.C = lsb(val);

			val = val >> 1;
			std::uint32_t msb_val = sr.X;
			msb_val = msb_val << (size_in_bytes(size) * 8 - 1);
			val = val | msb_val;

			sr.X = sr.C;
		}

		set_nz_flags(val, size, sr);
		sr.V = 0;
		return val;
	}

	template<class T1>
	static std::uint32_t lsl(T1 a, std::uint32_t shift_count, size_type size, status_register& sr)
	{
		std::uint32_t val = value(a, size);
		shift_count = shift_count % 64;

		if(shift_count == 0)
		{
			sr.C = 0;
		}
		else
		{
			sr.C = sr.X = msb(std::uint64_t(val) << (shift_count - 1), size);
		}

		val = std::uint64_t(val) << shift_count;
		val = value(val, size);

		sr.V = 0;
		set_nz_flags(val, size, sr);
		return val;
	}

	template<class T1>
	static std::uint32_t lsr(T1 a, std::uint32_t shift_count, size_type size, status_register& sr)
	{
		std::uint32_t val = value(a, size);
		shift_count = shift_count % 64;

		if(shift_count == 0)
		{
			sr.C = 0;
		}
		else
		{
			sr.C = sr.X = lsb(std::uint64_t(val) >> (shift_count - 1));
		}

		val = std::uint64_t(val) >> shift_count;
		val = value(val, size);

		sr.V = 0;
		set_nz_flags(val, size, sr);
		return val;
	}

	template<class T1>
	static void tst(T1 src, size_type size, status_register& sr)
	{
		sr.V = sr.C = 0;
		set_nz_flags(value(src, size), size, sr);
	}

	template<class T1>
	static std::uint32_t clr(T1 /* src */, size_type, status_register& sr)
	{
		sr.N = sr.V = sr.C = 0;
		sr.Z = 1;
		return 0;
	}

	template<class T1, class T2>
	static std::uint32_t mulu(T1 a, T2 b, status_register& sr)
	{
		std::uint32_t a_val = value(a, size_type::WORD);
		std::uint32_t b_val = value(b, size_type::WORD);
		std::uint32_t res = a_val * b_val;

		sr.V = sr.C = 0;
		set_nz_flags(res, size_type::LONG, sr);

		return res;
	}

	template<class T1, class T2>
	static std::uint32_t muls(T1 a, T2 b, status_register& sr)
	{
		std::int32_t a_val = sign_extend(value(a, size_type::WORD));
		std::int32_t b_val = sign_extend(value(b, size_type::WORD));
		std::int32_t res = a_val * b_val;

		sr.V = sr.C = 0;
		set_nz_flags(res, size_type::LONG, sr);

		return res;
	}

	static void divu_zero_division(status_register& sr)
	{
		sr.C = 0;
		// NOTE: these flags are undefined when zero division, but external tests expect to see 0 there
		sr.N = sr.V = sr.Z = 0;
	}

	template<class T1, class T2>
	static std::uint32_t divu(T1 dest, T2 src, status_register& sr)
	{
		std::uint32_t dest_val = value(dest, size_type::LONG);
		std::uint16_t src_val = value(src, size_type::WORD);

		sr.C = 0;
		bool is_overflow = (dest_val >> 16) >= src_val;
		if(is_overflow)
		{
			sr.V = 1;
			return dest_val;
		}

		std::uint16_t remainder = dest_val % src_val;
		std::uint16_t quotient = (dest_val - remainder) / src_val;

		std::uint32_t res = remainder;
		res = res << 16;
		res = res | quotient;

		sr.V = 0;
		set_nz_flags(quotient, size_type::WORD, sr);

		return res;
	}

	template<class T1, class T2>
	static std::uint32_t divs(T1 dest, T2 src, status_register& sr)
	{
		std::int32_t dest_val = value(dest, size_type::LONG);
		std::int16_t src_val = std::uint16_t(value(src, size_type::WORD));

		sr.C = 0;
		std::int32_t res = dest_val/src_val;
		bool is_overflow = res > std::numeric_limits<std::int16_t>::max()
			|| res < std::numeric_limits<std::int16_t>::min();
		if(is_overflow)
		{
			sr.V = 1;
			return dest_val;
		}

		std::int16_t remainder = dest_val % src_val;
		std::int16_t quotient = (dest_val - remainder) / src_val;

		res = std::uint16_t(remainder);
		res = res << 16;
		res = res | std::uint16_t(quotient);

		sr.V = 0;
		set_nz_flags(quotient, size_type::WORD, sr);

		return res;
	}

	template<class T1>
	static std::uint32_t ext(T1 a, size_type size, status_register& sr)
	{
		std::uint32_t res = 0;
		if(size == size_type::BYTE)
		{
			std::int8_t byte = value(a, size_type::BYTE);
			res = std::int16_t(byte);
			set_nz_flags(res, size_type::WORD, sr);
		}
		else
		{
			std::int16_t word = value(a, size_type::WORD);
			res = std::int32_t(word);
			set_nz_flags(res, size_type::LONG, sr);
		}

		sr.V = sr.C = 0;
		return res;
	}

	template<class T1, class T2>
	static void exg(T1& a, T2& b)
	{
		std::uint32_t a_val = value(a, size_type::LONG);
		std::uint32_t b_val = value(b, size_type::LONG);

		a.LW = b_val;
		b.LW = a_val;
	}

	template<class T1>
	static std::uint32_t swap(T1 a, status_register& sr)
	{
		std::uint32_t val = value(a, size_type::LONG);

		std::uint32_t lsw = val & 0xFFFF;
		std::uint32_t msw = val >> 16;

		std::uint32_t res = (lsw << 16) | msw;

		set_nz_flags(res, size_type::LONG, sr);
		sr.C = sr.V = 0;

		return res;
	}

	template<class T1>
	static std::uint8_t bit_number(T1 src, operand dest)
	{
		if(dest.is_data_reg())
			return value(src, size_type::LONG) % 32;
		return value(src, size_type::BYTE) % 8;
	}

	template<class T1>
	static void btst(T1 src, operand dest, status_register& sr)
	{
		std::uint32_t bit_num = bit_number(src, dest);
		std::uint32_t dest_val = bit_value(dest);

		bool bit_is_set = (dest_val >> bit_num) & 1;
		sr.Z = bit_is_set ? 0 : 1;
	}

	template<class T1>
	static std::uint32_t bset(T1 src, operand dest, status_register& sr)
	{
		std::uint32_t bit_num = bit_number(src, dest);
		std::uint32_t dest_val = bit_value(dest);

		bool bit_is_set = (dest_val >> bit_num) & 1;
		sr.Z = bit_is_set ? 0 : 1;

		std::uint32_t res = dest_val | (1 << bit_num);
		return res;
	}

	template<class T1>
	static std::uint32_t bclr(T1 src, operand dest, status_register& sr)
	{
		std::uint32_t bit_num = bit_number(src, dest);
		std::uint32_t dest_val = bit_value(dest);

		bool bit_is_set = (dest_val >> bit_num) & 1;
		sr.Z = bit_is_set ? 0 : 1;

		std::uint32_t res = dest_val & ~(1 << bit_num);
		return res;
	}

	template<class T1>
	static std::uint32_t bchg(T1 src, operand dest, status_register& sr)
	{
		std::uint32_t bit_num = bit_number(src, dest);
		std::uint32_t dest_val = bit_value(dest);

		bool bit_is_set = (dest_val >> bit_num) & 1;
		sr.Z = bit_is_set ? 0 : 1;

		std::uint32_t res;
		if(bit_is_set)
			res = dest_val & ~(1 << bit_num);
		else
			res = dest_val | (1 << bit_num);
		return res;
	}

	template<class T1, class T2>
	static bool chk(T1 src, T2 dest, status_register& sr)
	{
		std::int16_t src_val = std::uint16_t(value(src, size_type::WORD));
		std::int16_t dest_val = std::uint16_t(value(dest, size_type::WORD));

		bool lz = dest_val < 0;
		bool mu = dest_val > src_val;

		if(lz)
			sr.N = 1;
		else if(mu)
			sr.N = 0;

		// TODO: if exception is not reised, N is undefined, external tests expect to see undefined value
		// TODO: Z V C flags are undefined, howerver external tests expect to see 0
		sr.Z = sr.V = sr.C = 0;

		return lz || mu;
	}

	static bool cond_test(std::uint8_t cc, const status_register& sr)
	{
		cc = cc & 0b1111;
		switch (cc)
		{
		case 0b0000:
			return true;
		
		case 0b0001:
			return false;

		case 0b0010:
			return sr.C == 0 && sr.Z == 0;
		
		case 0b0011:
			return sr.C == 1 || sr.Z == 1;

		case 0b0100:
			return sr.C == 0;

		case 0b0101:
			return sr.C == 1;

		case 0b0110:
			return sr.Z == 0;

		case 0b0111:
			return sr.Z == 1;

		case 0b1000:
			return sr.V == 0;

		case 0b1001:
			return sr.V == 1;

		case 0b1010:
			return sr.N == 0;

		case 0b1011:
			return sr.N == 1;

		case 0b1100:
			return sr.N == sr.V;

		case 0b1101:
			return (sr.N == 1 && sr.V == 0) || (sr.N == 0 && sr.V == 1);

		case 0b1110:
			return (sr.Z == 0) && (sr.N == sr.V);

		case 0b1111:
			return (sr.Z == 1) || (sr.N == 1 && sr.V == 0) || (sr.N == 0 && sr.V == 1);

		default: throw internal_error();
		}
	}

	/* helpers */
	template<class T1, class T2>
	static std::uint32_t alu(inst_type inst, T1 a, T2 b, size_type size, status_register& sr)
	{
		switch (inst)
		{
		case inst_type::ADD:
		case inst_type::ADDI:
		case inst_type::ADDQ:
			return operations::add(a, b, size, sr);

		case inst_type::ADDA:
			return operations::adda(a, b, size, sr);

		case inst_type::ADDX:
			return operations::addx(a, b, size, sr);

		case inst_type::SUB:
		case inst_type::SUBI:
		case inst_type::SUBQ:
			return operations::sub(a, b, size, sr);

		case inst_type::SUBA:
			return operations::suba(a, b, size, sr);

		case inst_type::SUBX:
			return operations::subx(a, b, size, sr);

		case inst_type::AND:
		case inst_type::ANDI:
			return operations::and_op(a, b, size, sr);

		case inst_type::OR:
		case inst_type::ORI:
			return operations::or_op(a, b, size, sr);

		case inst_type::EOR:
		case inst_type::EORI:
			return operations::eor(a, b, size, sr);

		case inst_type::CMP:
		case inst_type::CMPI:
		case inst_type::CMPM:
			return operations::cmp(a, b, size, sr);

		case inst_type::CMPA:
			return operations::cmpa(a, b, size, sr);
		
		case inst_type::MULU:
			return operations::mulu(a, b, sr);
		
		case inst_type::MULS:
			return operations::muls(a, b, sr);

		case inst_type::DIVU:
			return operations::divu(a, b, sr);

		case inst_type::DIVS:
			return operations::divs(a, b, sr);

		default: throw internal_error();
		}
	}

	template<class T1>
	static std::uint32_t alu(inst_type inst, T1 a, size_type size, status_register& sr)
	{
		switch (inst)
		{
		case inst_type::NEG:
			return neg(a, size, sr);
		case inst_type::NEGX:
			return negx(a, size, sr);
		case inst_type::NOT:
			return not_op(a, size, sr);
		case inst_type::MOVE:
			return move(a, size, sr);
		case inst_type::CLR:
			return clr(a, (size_type)size, sr);

		default: throw internal_error();
		}
	}

	static void alu_to_sr(inst_type inst, std::uint16_t src, std::uint16_t& SR)
	{
		switch (inst)
		{
		case inst_type::ANDItoSR:
			andi_to_sr(src, SR);
			break;

		case inst_type::ORItoSR:
			ori_to_sr(src, SR);
			break;

		case inst_type::EORItoSR:
			eori_to_sr(src, SR);
			break;

		default: throw internal_error();
		}
	}

	static void alu_to_ccr(inst_type inst, std::uint8_t src, std::uint16_t& SR)
	{
		switch (inst)
		{
		case inst_type::ANDItoCCR:
			andi_to_ccr(src, SR);
			break;

		case inst_type::ORItoCCR:
			ori_to_ccr(src, SR);
			break;

		case inst_type::EORItoCCR:
			eori_to_ccr(src, SR);
			break;

		default: throw internal_error();
		}
	}

	template<class T1>
	static std::uint32_t shift(inst_type inst, T1 src, std::uint8_t shift_count, bool is_left_shift, size_type size, status_register& sr)
	{
		switch (inst)
		{
		case inst_type::ASLRreg:
		case inst_type::ASLRmem:
			if(is_left_shift)
				return asl(src, shift_count, size, sr);
			return asr(src, shift_count, size, sr);

		case inst_type::ROLRreg:
		case inst_type::ROLRmem:
			if(is_left_shift)
				return rol(src, shift_count, size, sr);
			return ror(src, shift_count, size, sr);

		case inst_type::LSLRreg:
		case inst_type::LSLRmem:
			if(is_left_shift)
				return lsl(src, shift_count, size, sr);
			return lsr(src, shift_count, size, sr);

		case inst_type::ROXLRreg:
		case inst_type::ROXLRmem:
			if(is_left_shift)
				return roxl(src, shift_count, size, sr);
			return roxr(src, shift_count, size, sr);

		default: throw internal_error();
		}
	}

	template<class T1>
	static std::uint32_t bit(inst_type inst, T1 src, operand dest, status_register& sr)
	{
		switch (inst)
		{
		case inst_type::BSETimm:
		case inst_type::BSETreg:
			return bset(src, dest, sr);

		case inst_type::BCLRimm:
		case inst_type::BCLRreg:
			return bclr(src, dest, sr);

		case inst_type::BCHGreg:
		case inst_type::BCHGimm:
			return bchg(src, dest, sr);

		default: throw internal_error();
		}
	}

	static std::uint32_t sign_extend(std::uint16_t val)
	{
		return std::int32_t(std::int16_t(val));
	}

	static std::uint16_t clear_unimplemented_flags(std::uint16_t SR)
	{
		const std::uint16_t implemented_flags_mask = 0b1010011100011111;
		return SR & implemented_flags_mask;
	}

private:
	static std::uint32_t add(std::uint32_t a, std::uint32_t b, std::uint8_t x, size_type size)
	{
		if(size == size_type::BYTE) return std::uint8_t(a + b + x);
		if(size == size_type::WORD) return std::uint16_t(a + b + x);
		return a + b + x;
	}

	static std::uint32_t add(std::uint32_t a, std::uint32_t b, size_type size, status_register& sr)
	{
		std::uint32_t res = add(a, b, 0, size);
		set_carry_and_overflow_flags(a, b, 0, size, sr);
		sr.X = sr.C;
		set_nz_flags(res, size, sr);
		return res;
	}

	static std::uint32_t addx(std::uint32_t a, std::uint32_t b, size_type size, status_register& sr)
	{
		std::uint32_t res = add(a, b, sr.X, size);
		set_carry_and_overflow_flags(a, b, sr.X, size, sr);
		sr.X = sr.C;
		if(res != 0) sr.Z = 0;
		sr.N = neg_flag(res, size);
		return res;
	}

	static std::uint32_t sub(std::uint32_t a, std::uint32_t b, std::uint8_t x, size_type size)
	{
		if(size == size_type::BYTE) return std::uint8_t(a - b - x);
		if(size == size_type::WORD) return std::uint16_t(a - b - x);
		return a - b - x;
	}

	static std::uint32_t sub(std::uint32_t a, std::uint32_t b, size_type size, status_register& sr)
	{
		std::uint32_t res = sub(a, b, 0, size);
		set_borrow_and_overflow_flags(a, b, 0, size, sr);
		sr.X = sr.C;
		set_nz_flags(res, size, sr);
		return res;
	}

	static std::uint32_t subx(std::uint32_t a, std::uint32_t b, size_type size, status_register& sr)
	{
		std::uint32_t res = sub(a, b, sr.X, size);
		set_borrow_and_overflow_flags(a, b, sr.X, size, sr);
		sr.X = sr.C;
		if(res != 0) sr.Z = 0;
		sr.N = neg_flag(res, size);
		return res;
	}

	static std::uint32_t and_op(std::uint32_t a, std::uint32_t b, size_type size, status_register& sr)
	{
		std::uint32_t res = a & b;
		set_logical_flags(res, size, sr);
		return res;
	}

	static std::uint32_t or_op(std::uint32_t a, std::uint32_t b, size_type size, status_register& sr)
	{
		std::uint32_t res = a | b;
		set_logical_flags(res, size, sr);
		return res;
	}

	static std::uint32_t eor(std::uint32_t a, std::uint32_t b, size_type size, status_register& sr)
	{
		std::uint32_t res = a ^ b;
		set_logical_flags(res, size, sr);
		return res;
	}

	static void set_carry_and_overflow_flags(std::uint32_t a, std::uint32_t b, std::uint8_t x,
		size_type size, status_register& sr)
	{
		if(size == size_type::BYTE)
		{
			sr.V = cpu_flags::overflow_add<std::int8_t>(a, b, x);
			sr.C = cpu_flags::carry<std::uint8_t>(a, b, x);
		}
		else if(size == size_type::WORD)
		{
			sr.V = cpu_flags::overflow_add<std::int16_t>(a, b, x);
			sr.C = cpu_flags::carry<std::uint16_t>(a, b, x);
		}
		else
		{
			sr.V = cpu_flags::overflow_add<std::int32_t>(a, b, x);
			sr.C = cpu_flags::carry<std::uint32_t>(a, b, x);
		}
	}

	static void set_borrow_and_overflow_flags(std::uint32_t a, std::uint32_t b, std::uint8_t x,
		size_type size, status_register& sr)
	{
		if(size == size_type::BYTE)
		{
			sr.V = cpu_flags::overflow_sub<std::int8_t>(a, b, x);
			sr.C = cpu_flags::borrow<std::uint8_t>(a, b, x);
		}
		else if(size == size_type::WORD)
		{
			sr.V = cpu_flags::overflow_sub<std::int16_t>(a, b, x);
			sr.C = cpu_flags::borrow<std::uint16_t>(a, b, x);
		}
		else
		{
			sr.V = cpu_flags::overflow_sub<std::int32_t>(a, b, x);
			sr.C = cpu_flags::borrow<std::uint32_t>(a, b, x);
		}
	}

	static void set_logical_flags(std::uint32_t res, size_type size, status_register& sr)
	{
		sr.C = sr.V = 0;
		set_nz_flags(res, size, sr);
	}

	static std::uint8_t neg_flag(std::uint32_t val, size_type size)
	{
		if(size == size_type::BYTE) return std::int8_t(val) < 0;
		if(size == size_type::WORD) return std::int16_t(val) < 0;
		return std::int32_t(val) < 0;
	}

	static std::uint8_t zer_flag(std::uint32_t val, size_type size)
	{
		return value(val, size) == 0;
	}

	static void set_nz_flags(std::uint32_t val, size_type size, status_register& sr)
	{
		sr.N = neg_flag(val, size);
		sr.Z = zer_flag(val, size);
	}

public:
	static std::uint32_t value(data_register reg, size_type size)
	{
		if(size == size_type::BYTE)
			return reg.B;
		else if(size == size_type::WORD)
			return reg.W;
		return reg.LW;
	}

	static std::uint32_t value(address_register reg, size_type size)
	{
		if(size == size_type::WORD)
			return reg.W;
		else if(size == size_type::LONG)
			return reg.LW;

		throw internal_error();
	}

	static std::uint32_t value(std::uint32_t val, size_type size)
	{
		if(size == size_type::BYTE)
			return val & 0xFF;
		if (size == size_type::WORD)
			return val & 0xFFFF;
		return val;
	}

	static std::uint32_t value(operand& op, size_type size)
	{
		if(op.is_imm()) return value(op.imm(), size);
		if(op.is_pointer()) return value(op.pointer().value(), size);
		if(op.is_data_reg()) return value(op.data_reg(), size);
		if(op.is_addr_reg()) return value(op.addr_reg(), size);

		throw internal_error();
	}

	static std::uint32_t bit_value(operand& dest)
	{
		if(dest.is_data_reg())
			return value(dest, size_type::LONG);
		return value(dest, size_type::BYTE);
	}

	// return most significant bit
	static std::uint8_t msb(std::uint32_t val, size_type size)
	{
		if(size == size_type::BYTE)
			return (val >> 7) & 1;
		if(size == size_type::WORD)
			return (val >> 15) & 1;
		return (val >> 31) & 1;
	}

	// return less significant bit
	static std::uint8_t lsb(std::uint32_t val)
	{
		return val & 1;
	}
};

}

#endif // __M68K_OPERATIONS_HPP__
