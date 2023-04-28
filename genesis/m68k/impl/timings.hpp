#ifndef __M68K_TIMINGS_HPP__
#define __M68K_TIMINGS_HPP__

#include <cstdint>
#include <cstdlib>
#include <bit>
#include "instruction_type.h"
#include "ea_decoder.hpp"


namespace genesis::m68k
{

/* Returns the amount of cycles per instruction except all read/write operations */
class timings
{
public:
	timings() = delete;

	/* ADD */
	static std::uint8_t add(std::uint8_t opmode, const operand& op)
	{
		if(opmode != 0b010 && opmode != 0b110)
			return 0;

		if(opmode == 0b110 && op.is_pointer())
			return 0;

		if(op.is_addr_reg() || op.is_data_reg() || op.is_imm())
			return 4;

		return 2;
	}

	static std::uint8_t addi(size_type size, const operand& op)
	{
		if(size != size_type::LONG || op.is_pointer())
			return 0;
		
		return 4;
	}

	static std::uint8_t addq(size_type size, const operand& op)
	{
		if(size == size_type::BYTE || op.is_pointer())
			return 0;

		if(size == size_type::WORD)
		{
			if(op.is_addr_reg())
				return 4;
			return 0;
		}

		// TODO: it looks like writing to address register also takes 4 extra cycles (not 2 as specified here)
		// however, there are 2 extra cycles in the test, so keep it 2 for now.
		if(op.is_data_reg() || op.is_imm())
			return 4;

		return 2;
	}
	
	static std::uint8_t adda(std::uint8_t opmode, const operand& op)
	{
		if(opmode == 0b011 || op.is_addr_reg() || op.is_data_reg() || op.is_imm())
			return 4;

		return 2;
	}

	/* SUB */
	static constexpr auto sub = add;
	static constexpr auto subi = addi;
	static constexpr auto subq = addq;
	static constexpr auto suba = adda;

	/* CMP */
	static std::uint8_t cmp(std::uint8_t opmode, const operand&)
	{
		if(opmode == 0b010)
			return 2;
		return 0;
	}

	static std::uint8_t cmpi(size_type size, const operand& op)
	{
		if(size == size_type::LONG && op.is_data_reg())
			return 2;
		return 0;
	}

	static std::uint8_t cmpa(std::uint8_t, const operand&)
	{
		return 2;
	}

	/* AND */
	static constexpr auto and_op = add;
	static constexpr auto andi = addi;

	/* OR */
	static constexpr auto or_op = add;
	static constexpr auto ori = addi;

	/* EOR */
	static constexpr auto eor = add;
	static constexpr auto eori = addi;
	
	/* NEG */
	static constexpr auto neg = cmpi;
	static constexpr auto negx = neg;

	/* NOT */
	static constexpr auto not_op = neg;

	/* MOVE */
	static std::uint8_t move_from_sr(const operand& op)
	{
		if(op.is_data_reg())
			return 2;
		return 0;
	}

	/* ASL/ASR/ROL/ROR/LSL/LSR */
	static std::uint16_t reg_shift(std::uint32_t shift_count, size_type size)
	{
		std::uint16_t cycles = 2 * (shift_count % 64) + 2;
		if(size == size_type::LONG)
			cycles += 2;
		return cycles;
	}

	/* CLR */
	static constexpr auto clr = cmpi;

	/* MULU/MULS */
	static std::uint8_t mulu(std::uint16_t src)
	{
		std::uint8_t ones = std::popcount(src);
		return (17 + ones) * 2;
	}

	static std::uint8_t muls(std::uint16_t val)
	{
		std::int32_t src = std::int16_t(val);
		src = src << 1;
		std::uint8_t m = 0;
		for(int i = 0; i < 17; ++i)
		{
			std::uint8_t lsb = src & 0b11;
			if(lsb == 0b10 || lsb == 0b01)
				++m;
			src = src >> 1;
		}

		return (17 + m) * 2;
	}

	static std::uint8_t divu(std::uint32_t dividend, std::uint16_t divisor)
	{
		bool is_overflow = (dividend >> 16) >= divisor;
		if(is_overflow)
			return 6;

		std::uint8_t cycles = 36;
		std::uint32_t div = divisor << 16;
		for(int i = 0; i < 15; ++i)
		{
			std::int32_t old_dividend = dividend;
			dividend = dividend << 1;

			if(old_dividend < 0)
			{
				dividend -= div;
			}
			else
			{
				if(dividend >= div)
				{
					dividend -= div;
					++cycles;
				}
				else
				{
					cycles += 2;
				}
			}
		}

		return cycles * 2;
	}

	static std::uint8_t divs(std::int32_t dividend, std::int16_t divisor)
	{
		std::uint8_t cycles = 4;

		if(dividend < 0)
			++cycles;

		std::int32_t sdiv = dividend/divisor;
		bool is_overflow = sdiv > std::numeric_limits<std::int16_t>::max()
			|| sdiv < std::numeric_limits<std::int16_t>::min();

		if(is_overflow)
			return (cycles + 2) * 2;

		cycles += 55;
		if(divisor >= 0)
		{
			if(dividend >= 0)
				--cycles;
			else
				++cycles;
		}

		std::uint32_t div = std::abs(dividend) / std::abs(divisor);
		for(int i = 0; i < 15; ++i)
		{
			if(std::int16_t(div) >= 0)
				++cycles;
			div = div << 1;
		}

		return cycles * 2;
	}

	static std::uint8_t exg()
	{
		return 2;
	}

	static std::uint8_t btst(const operand& op)
	{
		if(op.is_data_reg())
			return 2;

		// TODO: external tests expect to get this timing for imm operand
		// howerver, it's not documented
		if(op.is_imm())
			return 2;

		return 0;
	}

	static std::uint8_t bset(const operand& op, std::uint8_t bit_number)
	{
		if(op.is_data_reg())
			return bit_number < 16 ? 2 : 4;

		// TODO: external tests expect to get this timing for imm operand
		// howerver, it's not documented
		if(op.is_imm())
			return bit_number < 16 ? 2 : 4;

		return 0;
	}

	static std::uint8_t bclr(const operand& op, std::uint8_t bit_number)
	{
		if(op.is_data_reg())
			return bit_number < 16 ? 4 : 6;

		// TODO: external tests expect to get this timing for imm operand
		// howerver, it's not documented
		if(op.is_imm())
			return bit_number < 16 ? 4 : 6;

		return 0;
	}

	static constexpr auto bchg = bset;

	static std::uint8_t chk(std::int16_t src, std::int16_t reg)
	{
		if(reg > src)
			return 0 + 1; // TODO
		if(reg < 0)
			return 2 + 1; // TODO
		return 6;
	}

	static std::uint8_t bcc(bool test_result)
	{
		return test_result ? 2 : 4;
	}

	static std::uint8_t dbcc(bool test_result)
	{
		return test_result ? 4 : 2;
	}

	/* helpers */
	static std::uint8_t alu_mode(inst_type inst, std::uint8_t opmode, const operand& op)
	{
		switch (inst)
		{
		case inst_type::ADD:
			return add(opmode, op);
		case inst_type::ADDA:
			return adda(opmode, op);
		case inst_type::AND:
			return and_op(opmode, op);
		case inst_type::SUB:
			return sub(opmode, op);
		case inst_type::SUBA:
			return suba(opmode, op);
		case inst_type::OR:
			return or_op(opmode, op);
		case inst_type::EOR:
			return eor(opmode, op);
		case inst_type::CMP:
			return cmp(opmode, op);
		case inst_type::CMPA:
			return cmpa(opmode, op);

		default: throw internal_error();
		}
	}

	static std::uint8_t alu_size(inst_type inst, size_type size, const operand& op)
	{
		switch (inst)
		{
		case inst_type::ADDI:
			return addi(size, op);
		case inst_type::ADDQ:
			return addq(size, op);
		case inst_type::SUBI:
			return subi(size, op);
		case inst_type::SUBQ:
			return subq(size, op);
		case inst_type::ANDI:
			return andi(size, op);
		case inst_type::ORI:
			return ori(size, op);
		case inst_type::EORI:
			return eori(size, op);
		case inst_type::CMPI:
			return cmpi(size, op);
		case inst_type::NEG:
			return neg(size, op);
		case inst_type::NEGX:
			return negx(size, op);
		case inst_type::NOT:
			return not_op(size, op);
		case inst_type::CMPM:
			return 0;
		case inst_type::CLR:
			return clr(size, op);

		default: throw internal_error();
		}
	}

	static std::uint8_t mul(inst_type inst, std::uint16_t src)
	{
		switch (inst)
		{
		case inst_type::MULU:
			return mulu(src);

		case inst_type::MULS:
			return muls(src);

		default: throw internal_error();
		}
	}

	static std::uint8_t div(inst_type inst, std::uint32_t dividend, std::uint16_t divisor)
	{
		switch (inst)
		{
		case inst_type::DIVU:
			return divu(dividend, divisor);

		case inst_type::DIVS:
			return divs(dividend, divisor);

		default: throw internal_error();
		}
	}

	static std::uint8_t bit(inst_type inst, operand dest, std::uint8_t bit_number)
	{
		switch (inst)
		{
		case inst_type::BSETimm:
		case inst_type::BSETreg:
			return bset(dest, bit_number);

		case inst_type::BCLRimm:
		case inst_type::BCLRreg:
			return bclr(dest, bit_number);

		case inst_type::BCHGreg:
		case inst_type::BCHGimm:
			return bchg(dest, bit_number);

		default: throw internal_error();
		}
	}
};

}

#endif // __M68K_TIMINGS_HPP__
