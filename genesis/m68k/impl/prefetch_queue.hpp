#ifndef __M68K_PREFETCH_QUEUE_HPP__
#define __M68K_PREFETCH_QUEUE_HPP__

#include <string_view>

#include "bus_manager.hpp"
#include "m68k/cpu_registers.hpp"

namespace genesis::m68k
{

class prefetch_queue
{
private:
	enum fetch_state : std::uint8_t
	{
		IDLE,
		FETCH_ONE,
		FETCH_TWO,
		FETCH_IRC
	};

public:
	prefetch_queue(m68k::bus_manager& busm, m68k::cpu_registers& regs) : busm(busm), regs(regs)
	{
		IRC = IR = IRD = 0x0;
	}

	// Consider moving to cpu_registers
	std::uint16_t IRC;
	std::uint16_t IR;
	std::uint16_t IRD;

	bool is_idle() const
	{
		return state == IDLE;
	}

	void init_fetch_one()
	{
		assert_idle("init_fetch_one");

		busm.init_read_word(regs.PC);
		state = FETCH_ONE;
	}

	void init_fetch_two()
	{
		assert_idle("init_fetch_two");

		busm.init_read_word(regs.PC);
		state = FETCH_TWO;
	}

	void init_fetch_irc()
	{
		assert_idle("init_fetch_irc");

		busm.init_read_word(regs.PC);
		state = FETCH_IRC;
	}

	void cycle()
	{
		switch (state)
		{
		case IDLE:
			break;
		
		case FETCH_ONE:
			if(!busm.is_idle()) break;
			on_read_finished();
			state = IDLE; break;

		case FETCH_TWO:
			if(!busm.is_idle()) break;
			on_read_finished();
			busm.init_read_word(regs.PC);
			state = FETCH_ONE; break;
		
		case FETCH_IRC:
			if(!busm.is_idle()) break;
			on_read_finished(/* fill only IRC */ true);
			state = IDLE; break;

		default:
			break;
		}
	}

private:
	void on_read_finished(bool irc_only = false)
	{
		if(!irc_only) IR = IRC;
		IRC = busm.letched_word();
		if(!irc_only) IRD = IR;

		regs.PC += 2;
	}

	void assert_idle(std::string_view caller)
	{
		if(!is_idle())
		{
			throw std::runtime_error("prefetch_queue::" + std::string(caller) +
				" error: cannot start new prefetch while in the middle of the other");
		}
	}

private:
	m68k::bus_manager& busm;
	m68k::cpu_registers& regs;
	fetch_state state = IDLE;
};

}

#endif // __M68K_PREFETCH_QUEUE_HPP__