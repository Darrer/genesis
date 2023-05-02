#ifndef __M68K_EXCEPTION_UNIT_HPP__
#define __M68K_EXCEPTION_UNIT_HPP__

#include "base_unit.h"
#include "exception_manager.h"
#include "pc_corrector.hpp"
#include "prefetch_queue.hpp"


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
	exception_unit(m68k::cpu_registers& regs, exception_manager& exman, prefetch_queue& pq, cpu_bus& bus,
		m68k::bus_scheduler& scheduler, std::function<void()> __abort_execution,
		std::function<bool()> __instruction_unit_is_idle)
		: base_unit(regs, scheduler), exman(exman), pq(pq), bus(bus), __abort_execution(__abort_execution),
		__instruction_unit_is_idle(__instruction_unit_is_idle)
	{
		if(__abort_execution == nullptr)
			throw std::invalid_argument("__abort_execution");

		if(__instruction_unit_is_idle == nullptr)
			throw std::invalid_argument("__instruction_unit_is_idle");

		reset();
	}

	bool is_idle() const override
	{
		return base_unit::is_idle() &&
			!exception_0_group_is_rised() &&
			!exception_1_group_is_rised() &&
			!exception_2_group_is_rised();
	}

	void reset() override
	{
		state = IDLE;
		curr_ex = exception_type::none;
		base_unit::reset();
	}

protected:
	exec_state on_executing() override
	{
		switch (state)
		{
		case IDLE:
			accept_exception();
			state = EXECUTING;
			[[fallthrough]];

		case EXECUTING:
			// TODO: check if another exception is rised during executing the curr exception
			return exec();

		default:
			throw internal_error();
		}
	}

private:
	exec_state exec()
	{
		switch (curr_ex)
		{
	
		/* group 0 */

		case exception_type::reset:
			return reset_handler();

		case exception_type::address_error:
		case exception_type::bus_error:
			return address_error();

		/* group 1 */

		case exception_type::privilege_violations:
			return privilege_violations();

		/* group 2 */

		case exception_type::trap:
			return trap();

		case exception_type::trapv:
			return trapv();

		case exception_type::chk_instruction:
			return chk_instruction();

		case exception_type::division_by_zero:
			return division_by_zero();

		default: throw internal_error();
		}
	}

	// Checks if any of the following exceptions are reised:
	// 1. Reset
	// 2. Address error
	// 3. Bus error
	bool exception_0_group_is_rised() const
	{
		auto exps = { exception_type::reset, exception_type::address_error, exception_type::bus_error };
		return std::any_of(exps.begin(), exps.end(), [this](auto ex) { return exman.is_raised(ex); } );
	}

	// Checks if current executing instruction is over and any of the following exceptions are reised:
	// 1. Trace
	// 2. Interrupt
	// 3. Illegal
	// 4. Privilege
	bool exception_1_group_is_rised() const
	{
		if(!instruction_unit_is_idle())
			return false;

		auto exps = { exception_type::trace, exception_type::interrupt,
			exception_type::illegal_instruction, exception_type::privilege_violations };
		return std::any_of(exps.begin(), exps.end(), [this](auto ex) { return exman.is_raised(ex); } );
	}

	// Checks if current executing instruction is over and any of the following exceptions are reised:
	// 1. Trap
	// 2. Trapv
	// 3. CHK
	// 4. Zero Divide
	bool exception_2_group_is_rised() const
	{
		if(!instruction_unit_is_idle())
			return false;

		auto exps = { exception_type::trap, exception_type::trapv,
			exception_type::chk_instruction, exception_type::division_by_zero };
		return std::any_of(exps.begin(), exps.end(), [this](auto ex) { return exman.is_raised(ex); } );
	}

	void accept_exception()
	{
		if(accept_group_0())
			return;
		
		if(accept_group_1())
			return;
		
		if(accept_group_2())
			return;
		
		throw internal_error(); // why we were called?
	}

	bool accept_group_0()
	{
		if(exman.is_raised(exception_type::reset))
		{
			curr_ex = exception_type::reset;
			exman.accept_reset();
			return true;
		}

		if(exman.is_raised(exception_type::address_error))
		{
			curr_ex = exception_type::address_error;
			addr_error = exman.accept_address_error();
			return true;
		}

		if(exman.is_raised(exception_type::bus_error))
		{
			curr_ex = exception_type::bus_error;
			addr_error = exman.accept_bus_error();
			return true;
		}

		return false;
	}

	bool accept_group_1()
	{
		if(exman.is_raised(exception_type::trace))
		{
			throw not_implemented();
		}

		if(exman.is_raised(exception_type::interrupt))
		{
			throw not_implemented();
		}

		if(exman.is_raised(exception_type::illegal_instruction))
		{
			throw not_implemented();
		}

		if(exman.is_raised(exception_type::privilege_violations))
		{
			curr_ex = exception_type::privilege_violations;
			exman.accept_privilege_violations();
			return true;
		}

		return false;
	}

	bool accept_group_2()
	{
		if(exman.is_raised(exception_type::trap))
		{
			curr_ex = exception_type::trap;
			trap_vector = exman.accept_trap();
			return true;
		}

		if(exman.is_raised(exception_type::trapv))
		{
			curr_ex = exception_type::trapv;
			exman.accept_trapv();
			return true;
		}

		if(exman.is_raised(exception_type::chk_instruction))
		{
			curr_ex = exception_type::chk_instruction;
			exman.accept_chk_instruction();
			return true;
		}

		if(exman.is_raised(exception_type::division_by_zero))
		{
			curr_ex = exception_type::division_by_zero;
			exman.accept_division_by_zero();
			return true;
		}

		return false;
	}

	/*
		Note: Almost in all cases we start processing exception within 4 IDLE cycles,
		howerver, as we rise exceptoin on N cycle, but start processing it on N+1 cycle (e.g. on the next cycle)
		we wait for 3 cycles to compensate for this delay.
	*/

	exec_state reset_handler()
	{
		abort_execution();

		// update SR
		regs.flags.S = 1;
		regs.flags.TR = 0;

		// TODO: set interrupt priority mask at level 7

		// NOTE: this behavior is not clear for me
		// Set RESET and HALT for 10 cycles
		bus.set(bus::RESET);
		bus.set(bus::HALT);

		scheduler.wait(10);

		scheduler.call([this]()
		{
			bus.clear(bus::RESET);
			bus.clear(bus::HALT);
		});

		scheduler.wait(4);

		// TODO: read from supervisor program space

		// Read SSP
		scheduler.read(0, size_type::LONG, [this](std::uint32_t data, size_type)
		{
			regs.SSP.LW = data;
		});

		// Read PC
		scheduler.read(4, size_type::LONG, [this](std::uint32_t data, size_type)
		{
			regs.PC = data;
		});

		schedule_prefetch_two_with_gap();

		return exec_state::done;
	}

	/* Sequence of actions
	 * 1. Push PC
	 * 2. Push SR
	 * 3. Push Instruction Register (IRD)
	 * 4. Push address
	 * 5. Info word
	*/
	exec_state address_error()
	{
		if(!pq.is_idle())
		{
			// TODO: exception was rised by prefetch queue, in this case external tests
			// expect to see IN bit of status word set, couldn't find this behavior documented
			addr_error.in = true;
		}

		abort_execution();
		correct_pc();

		scheduler.wait(3);

		// PUSH PC LOW
		regs.SSP.LW -= 2;
		scheduler.write(regs.SSP.LW, regs.PC & 0xFFFF, size_type::WORD);

		// PUSH SR
		// note, for some reason we first push SR, then PC HIGH
		scheduler.write(regs.SSP.LW - 4, regs.SR, size_type::WORD);

		// update SR
		regs.flags.S = 1;
		regs.flags.TR = 0;

		// PUSH PC HIGH
		regs.SSP.LW -= 2;
		scheduler.write(regs.SSP.LW, regs.PC >> 16, size_type::WORD);
		regs.SSP.LW -= 2; // next word is already pushed on the stack

		// PUSH IRD
		regs.SSP.LW -= 2;
		scheduler.write(regs.SSP.LW, regs.SIRD, size_type::WORD);

		// PUSH address LOW
		regs.SSP.LW -= 2;
		scheduler.write(regs.SSP.LW, addr_error.address & 0xFFFF, size_type::WORD);

		// PUSH status word
		scheduler.write(regs.SSP.LW - 4, addr_error_info(), size_type::WORD);

		// PUSH address HIGH
		regs.SSP.LW -= 2;
		scheduler.write(regs.SSP.LW, addr_error.address >> 16, size_type::WORD);
		regs.SSP.LW -= 2; // next word is already pushed on the stack

		std::uint32_t addr = vector_address(curr_ex);
		scheduler.read(addr, size_type::LONG, [this](std::uint32_t data, size_type)
		{
			regs.PC = data;
		});

		schedule_prefetch_two_with_gap();
		
		return exec_state::done;
	}

	std::uint16_t addr_error_info() const
	{
		std::uint16_t status = regs.SIRD & ~0b11111; // undocumented behavior
		status |= addr_error.func_codes & 0x7; // first 3 bits

		if(addr_error.in)
			status |= 1 << 3; // 4rd bit
		
		if(addr_error.rw)
			status |= 1 << 4; // 5th bit

		return status;
	}

	exec_state trap()
	{
		scheduler.wait(3);
		schedule_trap(regs.PC, trap_vector);
		return exec_state::done;
	}

	exec_state trapv()
	{
		schedule_trap(regs.PC, 7);
		return exec_state::done;
	}

	exec_state division_by_zero()
	{
		scheduler.wait(7);
		schedule_trap(regs.SPC, 5);
		return exec_state::done;
	}

	exec_state privilege_violations()
	{
		throw not_implemented("implemented but not tested"); // TODO

		scheduler.wait(3);
		schedule_trap(regs.SPC, 8);
		return exec_state::done;
	}

	exec_state chk_instruction()
	{
		scheduler.wait(3);

		schedule_trap(regs.PC, 6);
		return exec_state::done;
	}

	/* Sequence of actions
	 * 1. Push PC
	 * 2. Push SR
	*/
	void schedule_trap(std::uint32_t pc, std::uint8_t trap_vector)
	{
		// PUSH PC LOW
		regs.SSP.LW -= 2;
		scheduler.write(regs.SSP.LW, pc & 0xFFFF, size_type::WORD);

		// PUSH SR
		// note, for some reason we first push SR, then PC HIGH
		scheduler.write(regs.SSP.LW - 4, regs.SR, size_type::WORD);

		// update SR
		regs.flags.S = 1;
		regs.flags.TR = 0;

		// PUSH PC HIGH
		regs.SSP.LW -= 2;
		scheduler.write(regs.SSP.LW, pc >> 16, size_type::WORD);
		regs.SSP.LW -= 2; // next word is already pushed on the stack

		std::uint32_t addr = trap_vector * 4;
		scheduler.read(addr, size_type::LONG, [this](std::uint32_t data, size_type)
		{
			regs.PC = data;
		});

		schedule_prefetch_two_with_gap();
	}

	static std::uint32_t vector_address(exception_type ex)
	{
		switch (ex)
		{
		case exception_type::bus_error:
			return 0x08;
		case exception_type::address_error:
			return 0x00C;

		default: throw internal_error();
		}
	}

	void correct_pc()
	{
		regs.PC = pc_corrector::correct_address_error(regs, addr_error);
	}

	void abort_execution()
	{
		__abort_execution();
	}

	bool instruction_unit_is_idle() const
	{
		return __instruction_unit_is_idle();
	}

	void schedule_prefetch_two_with_gap()
	{
		scheduler.prefetch_ird();
		scheduler.wait(2);
		scheduler.prefetch_irc();
	}

private:
	m68k::exception_manager& exman;
	prefetch_queue& pq;
	cpu_bus& bus;
	std::function<void()> __abort_execution;
	std::function<bool()> __instruction_unit_is_idle;
	exception_type curr_ex;
	ex_state state;

	m68k::address_error addr_error;
	std::uint8_t trap_vector;
};

}

#endif // __M68K_EXCEPTION_UNIT_HPP__
