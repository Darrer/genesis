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
		FETCH_IRD,
		FETCH_IRC,
		FETCH_ONE,
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

	// IR/IRD = (regs.PC)
	// IRC is not changed
	void init_fetch_ird()
	{
		assert_idle();

		busm.init_read_word(regs.PC, addr_space::PROGRAM, [this](){ on_complete(); });
		state = fetch_state::FETCH_IRD;
	}

	// IR/IRD aren't changed
	// IRC = (regs.PC + 2)
	void init_fetch_irc()
	{
		assert_idle();

		busm.init_read_word(regs.PC + 2, addr_space::PROGRAM, [this](){ on_complete(); });
		state = fetch_state::FETCH_IRC;
	}

	// IR/IRD = IRC
	// IRC = (regs.PC + 2)
	void init_fetch_one()
	{
		assert_idle();

		busm.init_read_word(regs.PC + 2, addr_space::PROGRAM, [this](){ on_complete(); });
		state = fetch_state::FETCH_ONE;
	}

private:
	void on_complete()
	{
		if(!busm.is_idle())
			throw internal_error("prefetch_queue::on_complete internal error: bus manager must be idle");

		std::uint16_t reg = busm.letched_word();

		switch (state)
		{
		case fetch_state::FETCH_IRD:
			regs.IRD = regs.IR = reg;
			break;

		case fetch_state::FETCH_IRC:
			regs.IRC = reg;
			break;

		case fetch_state::FETCH_ONE:
			regs.IRD = regs.IR = regs.IRC;
			regs.IRC = reg;
			break;

		default: throw internal_error();
		}

		reset();
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
