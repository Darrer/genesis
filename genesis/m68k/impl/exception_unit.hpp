#ifndef __M68K_EXCEPTION_UNIT_HPP__
#define __M68K_EXCEPTION_UNIT_HPP__

#include "base_unit.h"
#include "exception_manager.h"
#include "instruction_unit.hpp"

#include "exception.hpp"


namespace genesis::m68k
{

class exception_unit : public base_unit
{
private:
	enum ex_state
	{
		IDLE,
		EXECUTING,
	};

public:
	exception_unit(m68k::cpu_registers& regs, m68k::bus_manager& busm, m68k::prefetch_queue& pq,
		exception_manager& exman, m68k::instruction_unit& inst_unit)
		: base_unit(regs, busm, pq), exman(exman), inst_unit(inst_unit)
	{
		reset();
	}

	void reset() override
	{
		state = IDLE;
		curr_ex = exception_type::none;
		ex_stage = 0;

		base_unit::reset();
	}

	bool has_work() const
	{
		/* We should process these exceptions as soon as possible */
		if(exman.is_raised(exception_type::address_error))
			return true;

		return false;
	}

protected:
	void on_cycle() override
	{
		switch (state)
		{
		case IDLE:
			accept_exception();
			state = EXECUTING;
			[[fallthrough]];

		case EXECUTING:
			exec();
			break;

		default:
			throw internal_error();
		}
	}

private:
	void exec()
	{
		switch (curr_ex)
		{
		case exception_type::address_error:
			address_error();
			break;
	
		default: throw internal_error();
		}
	}

	void accept_exception()
	{
		if(exman.is_raised(exception_type::address_error))
		{
			curr_ex = exception_type::address_error;
			addr_error = exman.accept_address_error();
			inst_unit.reset();
			// pq.reset(); // TODO: we don't know who rised address error, so reset all components
		}
		else
		{
			throw not_implemented();
		}
	}

	/* Sequence of actions
	 * 1. Push PC
	 * 2. Push SR
	 * 3. Push Instruction Register (IRD)
	 * 4. Push address
	 * 5. Info word
	*/
	void address_error()
	{
		switch (ex_stage++)
		{
		case 0:
			wait(4);
			break;

		case 1:
			// PUSH PC LOW
			regs.SSP.LW -= 2;
			write_word(regs.SSP.LW, addr_error.PC & 0xFFFF);
			break;

		case 2:
			// PUSH SR
			// note, for some reason we first push SR, then PC HIGH
			write_word(regs.SSP.LW - 4, regs.SR);

			// update SR
			regs.flags.S = 1;
			regs.flags.TR = 0;
			break;

		case 3:
			// PUSH PC HIGH
			regs.SSP.LW -= 2;
			write_word(regs.SSP.LW, addr_error.PC >> 16);
			regs.SSP.LW -= 2; // next word is already pushed on the stack
			break;

		case 4:
			// PUSH IRD
			regs.SSP.LW -= 2;
			// TODO: IRD not always contains the current instruction
			write_word(regs.SSP.LW, pq.IRD);
			break;

		case 5:
			// PUSH address LOW
			regs.SSP.LW -= 2;
			write_word(regs.SSP.LW, addr_error.address & 0xFFFF);
			break;

		case 6:
			// PUSH status word
			write_word(regs.SSP.LW - 4, addr_error_info());
			break;

		case 7:
			// PUSH address HIGH
			regs.SSP.LW -= 2;
			write_word(regs.SSP.LW, addr_error.address >> 16);
			regs.SSP.LW -= 2; // next word is already pushed on the stack
			break;

		// fetch an exception routine address
		case 8:
			read_long(vector_address(exception_type::address_error));
			break;

		case 9:
			regs.PC = data;
			prefetch_two_and_idle();
			break;

		default: throw internal_error();
		}
	}

	std::uint16_t addr_error_info() const
	{
		// undocumented behavior
		std::uint16_t status = pq.IR & ~0b11111;
		status |= addr_error.func_codes & 0x7; // first 3 bits

		if(addr_error.in)
			status |= 1 << 3; // 3rd bit
		
		if(addr_error.rw)
			status |= 1 << 4; // 4th bit

		return status;
	}

	std::uint32_t vector_address(exception_type ex) const
	{
		switch (ex)
		{
		case exception_type::address_error:
			return 0x00C;

		default: throw internal_error();
		}
	}

private:
	m68k::exception_manager& exman;
	m68k::instruction_unit& inst_unit;
	exception_type curr_ex;
	ex_state state;
	std::uint16_t ex_stage;

	m68k::address_error addr_error;
};

}

#endif // __M68K_EXCEPTION_UNIT_HPP__
