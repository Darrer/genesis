#ifndef __M68K_PRIVILEGE_CHECKER_HPP__
#define __M68K_PRIVILEGE_CHECKER_HPP__

#include "instruction_type.h"
#include "m68k/cpu_registers.hpp"

namespace genesis::m68k::impl
{

class privilege_checker
{
public:
	privilege_checker() = delete;


	static bool is_authorized(m68k::inst_type inst, m68k::cpu_registers& regs)
	{
		if(regs.flags.S == 1)
			return true;

		switch (inst)
		{
		case inst_type::MOVEtoSR:
		case inst_type::MOVE_USP:
		case inst_type::ANDItoSR:
		case inst_type::ORItoSR:
		case inst_type::EORItoSR:
		case inst_type::RTE:
		case inst_type::RESET:
		// TODO: add STOP
			return false;

		default:
			return true;
		}
	}
};

}

#endif // __M68K_PRIVILEGE_CHECKER_HPP__