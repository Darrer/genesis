#include "base_unit.h"
#include "exception.hpp"


enum handler_state : std::uint8_t
{
	IDLE,
	EXECUTING,
	WAITING_SCHEDULER,
	WAITING_SCHEDULER_AND_IDLE,
};

namespace genesis::m68k
{

base_unit::base_unit(m68k::cpu_registers& regs, m68k::bus_scheduler& scheduler)
	: regs(regs), scheduler(scheduler)
{
	base_unit::reset();
}

void base_unit::reset()
{
	state = IDLE;
	imm = 0;
	data = 0;
}

bool base_unit::is_idle() const
{
	if(state == WAITING_SCHEDULER_AND_IDLE)
		return scheduler.is_idle();

	return state == IDLE;
}

void base_unit::cycle()
{
	if(state == WAITING_SCHEDULER || state == WAITING_SCHEDULER_AND_IDLE)
	{
		if(!scheduler.is_idle())
			return;

		state = state == WAITING_SCHEDULER ? EXECUTING : IDLE;
	}

	if(state == IDLE)
	{
		reset();
		state = EXECUTING;
	}

	if(state == EXECUTING)
	{
		executing();
		return;
	}

	// unreachable
	throw internal_error();
}

void base_unit::executing()
{
	while (true)
	{
		auto ex_state = on_executing();
		switch (ex_state)
		{
		case exec_state::wait_scheduler:
			if(scheduler.is_idle())
			{
				// scheduled nothing, repeat on_executing
				continue;
			}

			state = WAITING_SCHEDULER;
			return;

		case exec_state::done:
			if(scheduler.is_idle())
				state = IDLE;
			else
				state = WAITING_SCHEDULER_AND_IDLE;
			return;

		default: throw internal_error();
		}
	}
}

void base_unit::post_cycle()
{
	if(is_idle())
		state = IDLE;
}

void base_unit::read(std::uint32_t addr, std::uint8_t size)
{
	auto on_read = [this](std::uint32_t data, size_type)
	{
		this->data = data;
	};

	scheduler.read(addr, (size_type)size, on_read);
}

void base_unit::dec_and_read(std::uint8_t addr_reg, std::uint8_t size)
{
	if(size == size_type::BYTE || size == size_type::WORD)
	{
		regs.dec_addr(addr_reg, size);
		read(regs.A(addr_reg).LW, size);
	}
	else
	{
		regs.dec_addr(addr_reg, 2);

		this->reg_to_dec = addr_reg;
		
		auto on_read_lsw = [this](std::uint32_t data, size_type)
		{
			this->data = data;

			// we cannot dec in earler due to possible exceptions
			regs.dec_addr(reg_to_dec, 2);
		};
		scheduler.read(regs.A(addr_reg).LW, size_type::WORD, on_read_lsw);

		auto on_read_msw = [this](std::uint32_t data, size_type)
		{
			// we read MSW
			this->data |= data << 16;
		};
		scheduler.read(regs.A(addr_reg).LW - 2, size_type::WORD, on_read_msw);
	}
}

void base_unit::read_imm(std::uint8_t size)
{
	scheduler.read_imm((size_type)size, [this](std::uint32_t data, size_type)
	{
		this->imm = data;
	});

	state = WAITING_SCHEDULER;
}

}
