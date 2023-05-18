#include "base_unit.h"
#include "exception.hpp"


enum handler_state
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

		case exec_state::in_progress:
			state = EXECUTING;
			return;

		default: throw internal_error();
		}
	}
}

void base_unit::post_cycle()
{
	if(state == WAITING_SCHEDULER_AND_IDLE && scheduler.is_idle())
		state = IDLE;
}

void base_unit::read(std::uint32_t addr, size_type size)
{
	scheduler.read(addr, size, [this](std::uint32_t data, size_type)
	{
		this->data = data;
	});
}

void base_unit::dec_and_read(std::uint8_t addr_reg, size_type size)
{
	if(size == size_type::BYTE || size == size_type::WORD)
	{
		regs.dec_addr(addr_reg, size);
		read(regs.A(addr_reg).LW, size);
	}
	else
	{
		regs.dec_addr(addr_reg, size_type::WORD);

		// read LSW
		scheduler.read(regs.A(addr_reg).LW, size_type::WORD, [this](std::uint32_t data, size_type)
		{
			this->data = data;
		});

		scheduler.dec_addr_reg(addr_reg, size_type::WORD);

		// read MSW
		scheduler.read(regs.A(addr_reg).LW - 2, size_type::WORD, [this](std::uint32_t data, size_type)
		{
			this->data |= data << 16;
		});
	}
}

void base_unit::read_imm(size_type size)
{
	scheduler.read_imm(size, [this](std::uint32_t data, size_type)
	{
		this->imm = data;
	});
}

}
