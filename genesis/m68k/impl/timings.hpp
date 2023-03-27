#ifndef __M68K_TIMINGS_HPP__
#define __M68K_TIMINGS_HPP__

#include <cstdint>
#include "instruction_type.h"
#include "ea_decoder.hpp"

namespace genesis::m68k
{


/* Returns the amount of cycles per instruction after all read/write operations */
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

	static std::uint8_t addi(std::uint8_t size, const operand& op)
	{
		if(size != 4 || op.is_pointer())
			return 0;
		
		return 4;
	}

	static std::uint8_t addq(std::uint8_t size, const operand& op)
	{
		if(size == 1 || op.is_pointer())
			return 0;

		if(size == 2)
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

	static std::uint8_t cmpi(std::uint8_t size, const operand& op)
	{
		if(size == 4 && op.is_data_reg())
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

	/* NOT */
	static constexpr auto not_op = neg;

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

	static std::uint8_t alu_size(inst_type inst, std::uint8_t size, const operand& op)
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
		case inst_type::NOT:
			return not_op(size, op);

		default: throw internal_error();
		}
	}
};

}

#endif // __M68K_TIMINGS_HPP__
