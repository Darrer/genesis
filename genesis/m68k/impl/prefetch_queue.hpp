#ifndef __M68K_PREFETCH_QUEUE_HPP__
#define __M68K_PREFETCH_QUEUE_HPP__

#include "bus_manager.hpp"
#include "m68k/cpu_registers.hpp"


namespace genesis::m68k
{

class prefetch_queue
{
private:
	enum class fetch_state
	{
		IDLE,
		FETCH_ONE,
		FETCH_TWO,
		FETCH_TWO_WITH_GAP,
		FETCH_TWO_WITH_GAP_WAIT1,
		FETCH_TWO_WITH_GAP_WAIT2,
		FETCH_IRC
	};

public:
	prefetch_queue(m68k::bus_manager& busm, m68k::cpu_registers& regs) : busm(busm), regs(regs) { }

	bool is_idle() const
	{
		return state == fetch_state::IDLE;
	}

	void reset()
	{
		state = fetch_state::IDLE;
	}

	// IR/IRD = IRC
	// IRC = (regs.PC + 2)
	void init_fetch_one()
	{
		assert_idle();

		busm.init_read_word(regs.PC + 2, addr_space::PROGRAM, [&](){ on_complete(); });
		state = fetch_state::FETCH_ONE;
	}

	// IR/IRD = (regs.PC)
	// IRC = (regs.PC + 2)
	void init_fetch_two()
	{
		assert_idle();

		busm.init_read_word(regs.PC, addr_space::PROGRAM);
		state = fetch_state::FETCH_TWO;
	}

	// TODO: remove it
	// TODO: add instead init_fetch_ir - to prefetch at (regs.PC)
	// IR/IRD = (regs.PC)
	// IRC = (regs.PC + 2)
	// add 2 wait cycles between prefetches
	void init_fetch_two_with_gap()
	{
		assert_idle();

		busm.init_read_word(regs.PC, addr_space::PROGRAM);
		state = fetch_state::FETCH_TWO_WITH_GAP;
	}

	// IR/IRD aren't changed
	// IRC = (regs.PC + 2)
	void init_fetch_irc()
	{
		assert_idle();

		busm.init_read_word(regs.PC + 2, addr_space::PROGRAM, [&](){ on_complete(); });
		state = fetch_state::FETCH_IRC;
	}

	void cycle()
	{
		switch (state)
		{
		case fetch_state::IDLE:
			break;
		
		case fetch_state::FETCH_ONE:
		case fetch_state::FETCH_IRC:
			if(busm.is_idle())
				on_complete();
			break;

		case fetch_state::FETCH_TWO:
			if(busm.is_idle())
			{
				on_read_finished();
				busm.init_read_word(regs.PC + 2, addr_space::PROGRAM, [&](){ on_complete(); });
				state = fetch_state::FETCH_ONE;
			}
			break;

		case fetch_state::FETCH_TWO_WITH_GAP:
			if(busm.is_idle())
			{
				on_read_finished();
				state = fetch_state::FETCH_TWO_WITH_GAP_WAIT1;
			}
			break;

		case fetch_state::FETCH_TWO_WITH_GAP_WAIT1:
			state = fetch_state::FETCH_TWO_WITH_GAP_WAIT2;
			break;

		case fetch_state::FETCH_TWO_WITH_GAP_WAIT2:
			busm.init_read_word(regs.PC + 2, addr_space::PROGRAM, [&](){ on_complete(); });
			state = fetch_state::FETCH_ONE;
			break;

		default: throw internal_error();
		}
	}

private:
	void on_complete()
	{
		if(!busm.is_idle())
			throw internal_error("prefetch_queue::on_complete internal error: bus manager must be idle");

		if(state == fetch_state::FETCH_ONE)
		{
			on_read_finished();
		}
		else if(state == fetch_state::FETCH_IRC)
		{
			on_read_finished(/* set only IRC */ true);
		}
		else
		{
			state = fetch_state::IDLE;
			throw internal_error("prefetch_queue::on_complete internal error: unexpected state");
		}

		state = fetch_state::IDLE;
	}

	void on_read_finished(bool irc_only = false)
	{
		if(!irc_only)
			regs.IRD = regs.IR = regs.IRC;
		regs.IRC = busm.letched_word();
	}

	void assert_idle()
	{
		if(!is_idle())
		{
			throw internal_error("cannot start new prefetch while in the middle of the other");
		}
	}

private:
	m68k::bus_manager& busm;
	m68k::cpu_registers& regs;
	fetch_state state = fetch_state::IDLE;
};

}

#endif // __M68K_PREFETCH_QUEUE_HPP__
