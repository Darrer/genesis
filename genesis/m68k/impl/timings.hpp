#ifndef __M68K_TIMINGS_HPP__
#define __M68K_TIMINGS_HPP__

#include <cstdint>
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
		if(opmode != 0b010)
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

	/* AND */
	static constexpr auto and_op = add;
	static constexpr auto andi = addi;
};

}

#endif // __M68K_TIMINGS_HPP__
