#ifndef __M68K_PC_CORRECTOR_HPP__
#define __M68K_PC_CORRECTOR_HPP__

#include <cstdint>

#include "m68k/cpu_registers.hpp"
#include "opcode_decoder.h"
#include "exception_manager.h"


namespace genesis::m68k
{

// Correct PC before pushing on stack during processing exception
class pc_corrector
{
public:
	pc_corrector() = delete;

	static std::uint32_t correct_address_error(const cpu_registers& regs, const address_error& ex)
	{
		bool is_write = !ex.rw;
		std::uint16_t opcode = regs.SIRD;

		if(is_write && is_predec_move(opcode))
			return regs.PC;

		if(ex.in) // TODO: that's what external tests want
			return regs.PC - 4;

		return regs.PC - 2;
	}

private:
	static bool is_predec_move(std::uint16_t opcode)
	{
		bool is_move = opcode_decoder::decode(opcode) == inst_type::MOVE;
		if(is_move)
		{
			std::uint8_t mode = (opcode >> 6) & 0x7;
			if(mode == 0b100)
				return true;
		}

		return false;
	}
};


}

#endif // __M68K_PC_CORRECTOR_HPP__
