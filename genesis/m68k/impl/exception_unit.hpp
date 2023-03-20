#ifndef __M68K_EXCEPTION_UNIT_HPP__
#define __M68K_EXCEPTION_UNIT_HPP__

#include "base_unit.h"
#include "exception_manager.h"

#include "exception.hpp"


namespace genesis::m68k
{

enum exception_type
{
	none,
	address_error,
};

class exception_unit : public base_unit //, public exception_manager
{
private:
	enum ex_state
	{
		IDLE,
		EXECUTING,
	};

public:
	exception_unit(m68k::cpu_registers& regs, m68k::bus_manager& busm, m68k::prefetch_queue& pq)
		: base_unit(regs, busm, pq)
	{
		reset();
	}

	void reset() override
	{
		state = IDLE;
		ex_stage = 0;

		addr = 0;
		pc = 0;
		rw = 0;
		in = 0;

		base_unit::reset();
	}

	// void rise_address_error(std::uint32_t addr, std::uint32_t pc, std::uint8_t rw, std::uint8_t in) override
	// {
	// 	set_exception(exception_type::address_error);
	// 	this->addr = addr;
	// 	this->pc = pc;
	// 	this->rw = rw;
	// 	this->in = in;
	// }

protected:
	void on_cycle() override
	{
		switch (state)
		{
		case IDLE:
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
	
		default:
			throw internal_error();
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
			break;

		default: throw internal_error();
		}
	}

private:
	void set_exception(exception_type ex)
	{
		if(curr_ex != exception_type::none)
			throw not_implemented("nested exceptions are not supported yet");

		curr_ex = ex;
	}

private:
	exception_type curr_ex;
	ex_state state;
	std::uint16_t ex_stage;

	std::uint32_t addr;
	std::uint32_t pc;
	std::uint8_t rw;
	std::uint8_t in;
};

}

#endif // __M68K_EXCEPTION_UNIT_HPP__
