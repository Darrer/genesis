#ifndef __M68K_EXCEPTION_UNIT_HPP__
#define __M68K_EXCEPTION_UNIT_HPP__

#include "exception_manager.h"
#include "pc_corrector.hpp"
#include "exception.hpp"
#include "endian.hpp"


namespace genesis::m68k
{

class exception_unit
{
private:
	enum class ex_state
	{
		IDLE,
		WAITING_SCHEDULER,
	};

public:
	exception_unit(m68k::cpu_registers& regs, exception_manager& exman, cpu_bus& bus,
		m68k::bus_scheduler& scheduler, std::function<void()> __abort_execution,
		std::function<bool()> __instruction_unit_is_idle)
		: regs(regs), exman(exman), bus(bus), scheduler(scheduler), __abort_execution(__abort_execution),
		__instruction_unit_is_idle(__instruction_unit_is_idle)
	{
		if(__abort_execution == nullptr)
			throw std::invalid_argument("__abort_execution");

		if(__instruction_unit_is_idle == nullptr)
			throw std::invalid_argument("__instruction_unit_is_idle");

		reset();
	}

	void cycle()
	{
		if(state == ex_state::IDLE)
			process_exception();

		check_catastrophic_failures();
	}

	void post_cycle()
	{
		if(state == ex_state::WAITING_SCHEDULER && scheduler.is_idle())
			reset();
	}

	bool is_idle() const
	{
		if(state != ex_state::IDLE)
			return false;

		if(!exman.is_raised_any())
			return true;

		return !exception_0_group_is_rised() &&
			!exception_1_group_is_rised() &&
			!exception_2_group_is_rised();
	}

	void reset()
	{
		state = ex_state::IDLE;
		curr_ex = exception_type::none;
	}

private:
	void process_exception()
	{
		if(state != ex_state::IDLE || curr_ex != exception_type::none)
			throw internal_error();

		accept_exception();
		schedule_exception();
		state = ex_state::WAITING_SCHEDULER;
	}

	void schedule_exception()
	{
		switch (curr_ex)
		{
	
		/* group 0 */

		case exception_type::reset:
			reset_handler();
			break;

		case exception_type::address_error:
		case exception_type::bus_error:
			address_error();
			break;

		/* group 1 */

		case exception_type::trace:
			trace();
			break;

		case exception_type::interrupt:
			interrupt();
			break;

		case exception_type::illegal_instruction:
			illegal_instruction();
			break;

		case exception_type::privilege_violations:
			privilege_violations();
			break;

		case exception_type::line_1010_emulator:
			line_1010_emulator();
			break;

		case exception_type::line_1111_emulator:
			line_1111_emulator();
			break;

		/* group 2 */

		case exception_type::trap:
			trap();
			break;

		case exception_type::trapv:
			trapv();
			break;

		case exception_type::chk_instruction:
			chk_instruction();
			break;

		case exception_type::division_by_zero:
			division_by_zero();
			break;

		default: throw internal_error();
		}
	}

	// Checks if any of the following exceptions are raised:
	// 1. Reset
	// 2. Address error
	// 3. Bus error
	bool exception_0_group_is_rised() const
	{
		return exman.is_raised(exception_group::group_0);
	}

	// Checks if the current executing instruction is over and any of the following exceptions are raised:
	// 1. Trace
	// 2. Interrupt
	// 3. Illegal
	// 4. Line 1010 Emulator
	// 5. Line 1111 Emulator
	// 6. Privilege
	bool exception_1_group_is_rised() const
	{
		if(!instruction_unit_is_idle())
			return false;
		
		return exman.is_raised(exception_group::group_1);
	}

	// Checks if the current executing instruction is over and any of the following exceptions are raised:
	// 1. Trap
	// 2. Trapv
	// 3. CHK
	// 4. Zero Divide
	bool exception_2_group_is_rised() const
	{
		if(!instruction_unit_is_idle())
			return false;

		return exman.is_raised(exception_group::group_2);
	}

	void accept_exception()
	{
		if(accept_group(exception_group::group_0))
			return;

		/* In practice 2nd group has priority over 1st group */
		if(accept_group(exception_group::group_2))
			return;

		if(accept_group(exception_group::group_1))
			return;

		throw internal_error(); // why we were called?
	}

	// accept one exception from the group according its priority
	bool accept_group(exception_group group)
	{
		auto exps = group_exceptions(group);
		for(auto ex : exps)
		{
			if(accept(ex))
				return true;
		}
		return false;
	}

	bool accept(exception_type ex)
	{
		if(!exman.is_raised(ex))
			return false;

		curr_ex = ex;
		switch (curr_ex)
		{
		case exception_type::address_error:
			addr_error = exman.accept_address_error();
			break;

		case exception_type::bus_error:
			addr_error = exman.accept_bus_error();
			break;

		case exception_type::trap:
			trap_vector = exman.accept_trap();
			break;

		case exception_type::interrupt:
			if(bus.interrupt_priority() == 0)
				throw internal_error("interrupt exception is rised but there is no pending interrupt");
			exman.accept(curr_ex);
			break;

		default:
			exman.accept(curr_ex);
			break;
		}

		return true;
	}

	/*
		Note: Almost in all cases we start processing exception within 4 IDLE cycles,
		howerver, as we rise exceptoin on N cycle, but start processing it on N+1 cycle (i.e. on the next cycle)
		we wait for 3 cycles to compensate for this delay.
	*/

	void reset_handler()
	{
		abort_execution();
		exman.accept_all();

		// update SR
		regs.flags.S = 1;
		regs.flags.TR = 0;
		regs.flags.IPM = 7;

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

		// Read SSP
		scheduler.read(0, size_type::LONG, addr_space::PROGRAM, [this](std::uint32_t data, size_type)
		{
			regs.SSP.LW = data;
		});

		// Read PC
		scheduler.read(4, size_type::LONG, addr_space::PROGRAM, [this](std::uint32_t data, size_type)
		{
			regs.PC = data;
		});

		schedule_prefetch_two_with_gap();
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
		abort_execution();
		correct_pc();

		scheduler.wait(3);

		// PUSH PC LOW
		regs.SSP.LW -= 2;
		scheduler.write(regs.SSP.LW, endian::lsw(regs.PC), size_type::WORD);

		// PUSH SR
		// note, for some reason we first push SR, then PC HIGH
		scheduler.write(regs.SSP.LW - 4, regs.SR, size_type::WORD);

		// update SR
		regs.flags.S = 1;
		regs.flags.TR = 0;

		// PUSH PC HIGH
		regs.SSP.LW -= 2;
		scheduler.write(regs.SSP.LW, endian::msw(regs.PC), size_type::WORD);
		regs.SSP.LW -= 2; // next word is already pushed on the stack

		// PUSH IRD
		regs.SSP.LW -= 2;
		scheduler.write(regs.SSP.LW, regs.SIRD, size_type::WORD);

		// PUSH address LOW
		regs.SSP.LW -= 2;
		scheduler.write(regs.SSP.LW, endian::lsw(addr_error.address), size_type::WORD);

		// PUSH status word
		scheduler.write(regs.SSP.LW - 4, addr_error_info(), size_type::WORD);

		// PUSH address HIGH
		regs.SSP.LW -= 2;
		scheduler.write(regs.SSP.LW, endian::msw(addr_error.address), size_type::WORD);
		regs.SSP.LW -= 2; // next word is already pushed on the stack

		std::uint32_t addr = vector_address(curr_ex);
		scheduler.read(addr, size_type::LONG, [this](std::uint32_t data, size_type)
		{
			regs.PC = data;
		});

		schedule_prefetch_two_with_gap();
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

	void trace()
	{
		scheduler.wait(4);
		schedule_trap(regs.PC, vector_nummber(exception_type::trace));
	}

	void trap()
	{
		scheduler.wait(3);
		schedule_trap(regs.PC, trap_vector);
	}

	void trapv()
	{
		schedule_trap(regs.PC, vector_nummber(exception_type::trapv));
	}

	void division_by_zero()
	{
		scheduler.wait(7);
		schedule_trap(regs.SPC, vector_nummber(exception_type::division_by_zero));
	}

	void privilege_violations()
	{
		scheduler.wait(3);
		schedule_trap(regs.SPC, vector_nummber(exception_type::privilege_violations));
	}

	void chk_instruction()
	{
		scheduler.wait(3);
		schedule_trap(regs.PC, vector_nummber(exception_type::chk_instruction));
	}

	/* Sequence of actions
	 * 1. Push SR
	 * 2. Start Interrupt Acknowledge Cycle
	 * 3. Push PC (MSW first)
	 * 4. Read PC
	 * 5. Fill prefetch queue
	*/
	void interrupt()
	{
		scheduler.wait(6);

		scheduler.write(regs.SSP.LW - 6, regs.SR, size_type::WORD);

		// update SR
		regs.flags.S = 1;
		regs.flags.TR = 0;
		regs.flags.IPM = bus.interrupt_priority();

		scheduler.int_ack([this](std::uint8_t vector_number)
		{
			scheduler.wait(4);

			scheduler.write(regs.SSP.LW - 4, regs.PC, size_type::LONG, order::msw_first);
			regs.SSP.LW -= 6;

			std::uint32_t addr = vector_number * 4;
			scheduler.read(addr, size_type::LONG, [this](std::uint32_t data, size_type)
			{
				regs.PC = data;
			});

			schedule_prefetch_two_with_gap();
		});
	}

	void illegal_instruction()
	{
		scheduler.wait(4);
		schedule_trap(regs.PC, vector_nummber(exception_type::illegal_instruction));
	}

	void line_1010_emulator()
	{
		scheduler.wait(4);
		schedule_trap(regs.PC, vector_nummber(exception_type::line_1010_emulator));
	}

	void line_1111_emulator()
	{
		scheduler.wait(4);
		schedule_trap(regs.PC, vector_nummber(exception_type::line_1111_emulator));
	}

	/* Sequence of actions
	 * 1. Push PC
	 * 2. Push SR
	*/
	void schedule_trap(std::uint32_t pc, std::uint8_t trap_vector)
	{
		// PUSH PC LOW
		regs.SSP.LW -= 2;
		scheduler.write(regs.SSP.LW, endian::lsw(pc), size_type::WORD);

		// PUSH SR
		// note, we first push SR, then PC HIGH
		scheduler.write(regs.SSP.LW - 4, regs.SR, size_type::WORD);

		// update SR
		regs.flags.S = 1;
		regs.flags.TR = 0;

		// PUSH PC HIGH
		regs.SSP.LW -= 2;
		scheduler.write(regs.SSP.LW, endian::msw(pc), size_type::WORD);
		regs.SSP.LW -= 2; // next word is already pushed on the stack

		std::uint32_t addr = trap_vector * 4;
		scheduler.read(addr, size_type::LONG, [this](std::uint32_t data, size_type)
		{
			regs.PC = data;
		});

		schedule_prefetch_two_with_gap();
	}

	static std::uint32_t vector_nummber(exception_type ex)
	{
		switch (ex)
		{
		case exception_type::bus_error:
			return 2;
		case exception_type::address_error:
			return 3;
		case exception_type::illegal_instruction:
			return 4;
		case exception_type::division_by_zero:
			return 5;
		case exception_type::chk_instruction:
			return 6;
		case exception_type::trapv:
			return 7;
		case exception_type::privilege_violations:
			return 8;
		case exception_type::trace:
			return 9;
		case exception_type::line_1010_emulator:
			return 10;
		case exception_type::line_1111_emulator:
			return 11;
		default: throw internal_error();
		}
	}

	static std::uint32_t vector_address(exception_type ex)
	{
		return vector_nummber(ex) * 4;
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

	void check_catastrophic_failures() const
	{
		/** Catastrophic failures can occur when during processing bus/address error exception
		  * another bus/address error exception is occured.
		  * CPU should be halted in this case.
		*/

		if(!exman.is_raised_any())
			return;

		if(exman.is_raised(exception_type::address_error) || exman.is_raised(exception_type::bus_error))
		{
			if(curr_ex == exception_type::address_error || curr_ex == exception_type::bus_error)
			{
				// TODO: set CPU to halted state
				throw std::runtime_error("CPU halted");
			}
		}
	}

private:
	m68k::cpu_registers& regs;
	m68k::exception_manager& exman;
	m68k::cpu_bus& bus;
	m68k::bus_scheduler& scheduler;
	std::function<void()> __abort_execution;
	std::function<bool()> __instruction_unit_is_idle;
	exception_type curr_ex;
	ex_state state;

	m68k::address_error addr_error;
	std::uint8_t trap_vector;
};

}

#endif // __M68K_EXCEPTION_UNIT_HPP__
