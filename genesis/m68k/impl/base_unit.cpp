#include "base_unit.h"

#include "exception.hpp"

#include <iostream>

enum handler_state : std::uint8_t
{
	IDLE,
	EXECUTING,
	SCHEDULER,
};

namespace genesis::m68k
{

base_unit::base_unit(m68k::cpu_registers& regs, m68k::bus_manager& busm,
	m68k::prefetch_queue& pq, m68k::bus_scheduler& scheduler):
	regs(regs), busm(busm), pq(pq), scheduler(scheduler)
{
	base_unit::reset();
}

void base_unit::reset()
{
	state = IDLE;
	imm = 0;
	data = 0;
	go_idle = false;
}

bool base_unit::is_idle() const
{
	if(state == SCHEDULER && go_idle)
		return scheduler.is_idle();

	return state == IDLE;
}

void base_unit::cycle()
{
	if(state == SCHEDULER)
	{
		if(!scheduler.is_idle())
			return;

		state = go_idle ? IDLE : EXECUTING;
	}

	if(state == IDLE)
	{
		reset();
		state = EXECUTING;
	}

	if(state == EXECUTING)
	{
		exec();
		return;
	}

	// unreachable
	throw internal_error();
}

void base_unit::exec()
{
	while (true)
	{
		auto req = on_handler();

		switch (req)
		{
		case handler::wait_scheduler:
			if(scheduler.is_idle())
			{
				// scheduled nothing, repeat now
				continue;
			}

			wait_scheduler();
			return;

		case handler::wait_scheduler_and_done:
			wait_scheduler_and_idle();
			return;
		
		case handler::in_progress:
			return; // just wait
		
		default: throw internal_error();
		}
	}
}

void base_unit::post_cycle()
{
	if(is_idle())
		state = IDLE;
}

void base_unit::idle()
{
	state = IDLE;
}

void base_unit::read(std::uint32_t addr, std::uint8_t size, bus_manager::on_complete cb)
{
	if(size == 1)
		read_byte(addr, cb);
	else if(size == 2)
		read_word(addr, cb);
	else
		read_long(addr, cb);
}

void base_unit::dec_and_read(std::uint8_t addr_reg, std::uint8_t size, bus_manager::on_complete cb)
{
	if(size == 4)
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

		state = SCHEDULER;

		return;
	}

	regs.dec_addr(addr_reg, size);
	std::uint32_t addr = regs.A(addr_reg).LW;

	if(size == 2)
	{
		read_word(addr, cb);
	}
	else
	{
		read_byte(addr, cb);
	}
}

void base_unit::read_byte(std::uint32_t addr, bus_manager::on_complete cb)
{
	this->cb = cb;
	auto on_complete = [this](std::uint32_t data, size_type)
	{
		this->data = data;
		if(this->cb) this->cb();
	};

	scheduler.read(addr, size_type::BYTE, on_complete);
	state = SCHEDULER;
}

void base_unit::read_word(std::uint32_t addr, bus_manager::on_complete cb)
{
	this->cb = cb;
	auto on_complete = [this](std::uint32_t data, size_type)
	{
		this->data = data;
		if(this->cb) this->cb();
	};

	scheduler.read(addr, size_type::WORD, on_complete);
	state = SCHEDULER;
}

void base_unit::read_long(std::uint32_t addr, bus_manager::on_complete cb)
{
	this->cb = cb;
	auto on_complete = [this](std::uint32_t data, size_type)
	{
		this->data = data;
		if(this->cb) this->cb();
	};

	scheduler.read(addr, size_type::LONG, on_complete);
	state = SCHEDULER;
}

void base_unit::read_imm(std::uint8_t size, bus_manager::on_complete cb)
{
	auto on_complete = [this](std::uint32_t data, size_type)
	{
		this->imm = data;
	};

	scheduler.read_imm((size_type)size, on_complete);
	state = SCHEDULER;
}

void base_unit::wait_scheduler()
{
	state = SCHEDULER;
}

void base_unit::wait_scheduler_and_idle()
{
	wait_scheduler();
	go_idle = true;
}

}
