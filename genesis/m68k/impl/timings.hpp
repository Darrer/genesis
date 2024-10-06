#ifndef __M68K_TIMINGS_HPP__
#define __M68K_TIMINGS_HPP__

#include "ea_decoder.hpp"
#include "instruction_type.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <limits>


namespace genesis::m68k
{

/* Returns the amount of cycles per instruction except all read/write operations */
class timings
{
public:
	timings() = delete;

	/* ADD */
	static constexpr std::uint8_t add(addressing_mode mode, std::uint8_t opmode)
	{
		if(opmode != 0b010 && opmode != 0b110)
			return 0;

		bool indir_mode = indirect_mode(mode);
		if(opmode == 0b110 && indir_mode)
			return 0;

		if(!indir_mode)
			return 4;

		return 2;
	}

	static constexpr std::uint8_t addi(addressing_mode mode, size_type size)
	{
		if(size != size_type::LONG || indirect_mode(mode))
			return 0;
		return 4;
	}

	static constexpr std::uint8_t addq(addressing_mode mode, size_type size)
	{
		if(size == size_type::BYTE || indirect_mode(mode))
			return 0;

		if(size == size_type::WORD)
		{
			return mode == addressing_mode::addr_reg ? 4 : 0;
		}

		// TODO: it looks like writing to address register also takes 4 extra cycles (not 2 as specified here)
		// however, there are 2 extra cycles in the test, so keep it 2 for now.
		if(mode == addressing_mode::data_reg || mode == addressing_mode::imm)
			return 4;
		return 2;
	}

	static constexpr std::uint8_t adda(addressing_mode mode, std::uint8_t opmode)
	{
		if(opmode == 0b011 || !indirect_mode(mode))
			return 4;

		return 2;
	}

	/* SUB */
	static constexpr auto sub = add;
	static constexpr auto subi = addi;
	static constexpr auto subq = addq;
	static constexpr auto suba = adda;

	/* CMP */
	static constexpr std::uint8_t cmp(std::uint8_t opmode)
	{
		// TODO: provide size, not opmode?
		return opmode == 0b010 ? 2 : 0;
	}

	static constexpr std::uint8_t cmpi(addressing_mode mode, size_type size)
	{
		if(size == size_type::LONG && mode == addressing_mode::data_reg)
			return 2;
		return 0;
	}

	static constexpr std::uint8_t cmpa()
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
	static constexpr std::uint8_t move_from_sr(addressing_mode mode)
	{
		return mode == addressing_mode::data_reg ? 2 : 0;
	}

	static constexpr std::uint8_t move_to_sr()
	{
		return 4;
	}

	static constexpr std::uint8_t move_to_ccr()
	{
		return 4;
	}

	/* ASL/ASR/ROL/ROR/LSL/LSR */
	static constexpr std::uint8_t reg_shift(std::uint32_t shift_count, size_type size)
	{
		std::uint8_t cycles = 2 * (shift_count % 64) + 2;
		if(size == size_type::LONG)
			cycles += 2;
		return cycles;
	}

	/* CLR */
	static constexpr auto clr = cmpi;

	/* MULU/MULS */
	static constexpr std::uint8_t mulu(std::uint16_t src)
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
		bool is_overflow = endian::msw(dividend) >= divisor;
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

		std::int32_t sdiv = dividend / divisor;
		bool is_overflow =
			sdiv > std::numeric_limits<std::int16_t>::max() || sdiv < std::numeric_limits<std::int16_t>::min();

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

	static constexpr std::uint8_t exg()
	{
		return 2;
	}

	static constexpr std::uint8_t btst(addressing_mode mode)
	{
		if(mode == addressing_mode::data_reg)
			return 2;

		// TODO: external tests expect to get this timing for imm operand
		// howerver, it's not documented
		if(mode == addressing_mode::imm)
			return 2;

		return 0;
	}

	static constexpr std::uint8_t bset(addressing_mode mode, std::uint8_t bit_number)
	{
		if(mode == addressing_mode::data_reg)
			return bit_number < 16 ? 2 : 4;

		// TODO: external tests expect to get this timing for imm operand
		// howerver, it's not documented
		if(mode == addressing_mode::imm)
			return bit_number < 16 ? 2 : 4;

		return 0;
	}

	static constexpr std::uint8_t bclr(addressing_mode mode, std::uint8_t bit_number)
	{
		if(mode == addressing_mode::data_reg)
			return bit_number < 16 ? 4 : 6;

		// TODO: external tests expect to get this timing for imm operand
		// howerver, it's not documented
		if(mode == addressing_mode::imm)
			return bit_number < 16 ? 4 : 6;

		return 0;
	}

	static constexpr auto bchg = bset;

	static constexpr std::uint8_t chk(std::int16_t src, std::int16_t reg)
	{
		if(reg > src)
			return 0 + 1; // TODO
		if(reg < 0)
			return 2 + 1; // TODO
		return 6;
	}

	static constexpr std::uint8_t bcc(bool test_result)
	{
		return test_result ? 2 : 4;
	}

	static constexpr std::uint8_t dbcc(bool test_result)
	{
		return test_result ? 4 : 2;
	}

	static constexpr std::uint8_t scc(bool test_result, addressing_mode mode)
	{
		if(test_result && mode == addressing_mode::data_reg)
			return 2;
		return 0;
	}

	static constexpr std::uint8_t bcd_reg()
	{
		return 2;
	}

	static constexpr std::uint8_t bsr()
	{
		return 2;
	}

	static constexpr std::uint8_t nbcd(addressing_mode mode)
	{
		return mode == addressing_mode::data_reg ? 2 : 0;
	}

	static constexpr std::uint8_t reset()
	{
		return 128;
	}

	static constexpr std::uint8_t lea(addressing_mode mode)
	{
		if(mode == addressing_mode::index_indir || mode == addressing_mode::index_pc)
			return 2;
		return 0;
	}

	static constexpr auto pea = lea;

	/* helpers */
	static std::uint8_t alu_mode(inst_type inst, addressing_mode mode, std::uint8_t opmode)
	{
		switch(inst)
		{
		case inst_type::ADD:
			return add(mode, opmode);
		case inst_type::ADDA:
			return adda(mode, opmode);
		case inst_type::AND:
			return and_op(mode, opmode);
		case inst_type::SUB:
			return sub(mode, opmode);
		case inst_type::SUBA:
			return suba(mode, opmode);
		case inst_type::OR:
			return or_op(mode, opmode);
		case inst_type::EOR:
			return eor(mode, opmode);
		case inst_type::CMP:
			return cmp(opmode);
		case inst_type::CMPA:
			return cmpa();

		default:
			throw internal_error();
		}
	}

	static std::uint8_t alu_size(inst_type inst, addressing_mode mode, size_type size)
	{
		switch(inst)
		{
		case inst_type::ADDI:
			return addi(mode, size);
		case inst_type::ADDQ:
			return addq(mode, size);
		case inst_type::SUBI:
			return subi(mode, size);
		case inst_type::SUBQ:
			return subq(mode, size);
		case inst_type::ANDI:
			return andi(mode, size);
		case inst_type::ORI:
			return ori(mode, size);
		case inst_type::EORI:
			return eori(mode, size);
		case inst_type::CMPI:
			return cmpi(mode, size);
		case inst_type::NEG:
			return neg(mode, size);
		case inst_type::NEGX:
			return negx(mode, size);
		case inst_type::NOT:
			return not_op(mode, size);
		case inst_type::CMPM:
			return 0;
		case inst_type::CLR:
			return clr(mode, size);
		case inst_type::NBCD:
			return nbcd(mode);

		default:
			throw internal_error();
		}
	}

	static std::uint8_t mul(inst_type inst, std::uint16_t src)
	{
		switch(inst)
		{
		case inst_type::MULU:
			return mulu(src);

		case inst_type::MULS:
			return muls(src);

		default:
			throw internal_error();
		}
	}

	static std::uint8_t div(inst_type inst, std::uint32_t dividend, std::uint16_t divisor)
	{
		switch(inst)
		{
		case inst_type::DIVU:
			return divu(dividend, divisor);

		case inst_type::DIVS:
			return divs(dividend, divisor);

		default:
			throw internal_error();
		}
	}

	static std::uint8_t bit(inst_type inst, addressing_mode mode, std::uint8_t bit_number)
	{
		switch(inst)
		{
		case inst_type::BSETimm:
		case inst_type::BSETreg:
			return bset(mode, bit_number);

		case inst_type::BCLRimm:
		case inst_type::BCLRreg:
			return bclr(mode, bit_number);

		case inst_type::BCHGreg:
		case inst_type::BCHGimm:
			return bchg(mode, bit_number);

		default:
			throw internal_error();
		}
	}

private:
	static constexpr bool indirect_mode(addressing_mode mode)
	{
		if(mode == addressing_mode::data_reg || mode == addressing_mode::addr_reg || mode == addressing_mode::imm)
			return false;
		return true;
	}
};

} // namespace genesis::m68k

#endif // __M68K_TIMINGS_HPP__
