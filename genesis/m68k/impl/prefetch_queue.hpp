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
	using on_complete = std::function<void()>;

private:
	// all callbacks are restricted in size to the size of the pointer
	// this is required for std::function small-size optimizations
	// (though it's not guaranteed by standard, so we purely rely on implementation)
	constexpr const static std::size_t max_callable_size = sizeof(void*);

public:
	prefetch_queue(m68k::bus_manager& busm, m68k::cpu_registers& regs) : busm(busm), regs(regs) { }

	bool is_idle() const
	{
		return state == fetch_state::IDLE;
	}

	void reset()
	{
		state = fetch_state::IDLE;
		on_complete_cb = nullptr;
	}

	// IR/IRD = (regs.PC)
	// IRC is not changed
	template<class Callable = std::nullptr_t>
	void init_fetch_ird(Callable cb = nullptr)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		assert_idle();

		busm.init_read_word(regs.PC, addr_space::PROGRAM, [this](){ on_read_finished(); });
		state = fetch_state::FETCH_IRD;
		on_complete_cb = cb;
	}

	// IR/IRD aren't changed
	// IRC = (regs.PC + 2)
	template<class Callable = std::nullptr_t>
	void init_fetch_irc(Callable cb = nullptr)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		assert_idle();

		busm.init_read_word(regs.PC + 2, addr_space::PROGRAM, [this](){ on_read_finished(); });
		state = fetch_state::FETCH_IRC;
		on_complete_cb = cb;
	}

	// IR/IRD = IRC
	// IRC = (regs.PC + 2)
	template<class Callable = std::nullptr_t>
	void init_fetch_one(Callable cb = nullptr)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		assert_idle();

		busm.init_read_word(regs.PC + 2, addr_space::PROGRAM, [this](){ on_read_finished(); });
		state = fetch_state::FETCH_ONE;
		on_complete_cb = cb;
	}

private:
	void on_read_finished()
	{
		auto reg = busm.latched_word();

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

		if(on_complete_cb != nullptr)
			on_complete_cb();

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
	on_complete on_complete_cb;
};

}

#endif // __M68K_PREFETCH_QUEUE_HPP__
