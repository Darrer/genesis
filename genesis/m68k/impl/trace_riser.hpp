#ifndef __M68K_TRACE_RISER_HPP__
#define __M68K_TRACE_RISER_HPP__

#include <functional>
#include <stdexcept>

#include "exception_manager.h"
#include "m68k/cpu_registers.hpp"

namespace genesis::m68k::impl
{

/* Rises trace exception on instruction completion */
// NOTE: implemented but not tested
class trace_riser
{
public:
	trace_riser(m68k::cpu_registers& regs, m68k::exception_manager& exman, std::function<bool()> __instruction_unit_is_idle) :
		regs(regs), exman(exman), __instruction_unit_is_idle(__instruction_unit_is_idle)
	{
		if(__instruction_unit_is_idle == nullptr)
			throw std::invalid_argument("__instruction_unit_is_idle");
	}

	void reset()
	{
		executing = false;
		tracing_is_enabled = false;
	}

	/* Must be called in the begining of cycle */
	void cycle()
	{
		// We have to save TR flag before executing instruction
		// and rise trace based on saved value
		if(instruction_unit_is_idle())
			tracing_is_enabled = regs.flags.TR == 1;
	}

	/* Must be called in the end of cycle */
	void post_cycle()
	{
		bool inst_unit_is_idle = instruction_unit_is_idle();

		// Transition executing -> idle
		if(executing && inst_unit_is_idle)
		{
			rise_trace_if_required();
		}

		executing = !inst_unit_is_idle;
	}

private:
	bool instruction_unit_is_idle() const
	{
		return __instruction_unit_is_idle();
	}

	void rise_trace_if_required()
	{
		if(!tracing_is_enabled)
			return;

		// don't rise if any of the following exceptions occurs during executing instruction
		if(exman.is_raised(exception_type::illegal_instruction) || exman.is_raised(exception_type::privilege_violations))
			return;

		exman.rise_trace();
	}

private:
	m68k::cpu_registers& regs;
	m68k::exception_manager& exman;
	std::function<bool()> __instruction_unit_is_idle;
	bool executing = false;
	bool tracing_is_enabled = false;
};

};


#endif // __M68K_TRACE_RISER_HPP__
