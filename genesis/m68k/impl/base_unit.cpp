#include "base_unit.h"

#include "exception.hpp"

#include <iostream>

enum handler_state : std::uint8_t
{
	IDLE,
	EXECUTING,
	SCHEDULER,
	WAIT_SCHEDULER,
	WAITING,
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
	cycles_to_wait = 0;
	cycles_after_idle = 0;
	go_idle = false;
}

bool base_unit::is_idle() const
{
	if(cycles_after_idle > 0)
		return false;

	if(state == SCHEDULER && go_idle)
		return scheduler.is_idle();

	if(state == WAITING && go_idle)
		return cycles_to_wait == 0;

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

	if(state == WAITING)
	{
		if(cycles_to_wait > 0)
		{
			--cycles_to_wait;
			return;
		}

		state = go_idle ? IDLE : EXECUTING;
	}

	if(state == IDLE)
	{
		if(cycles_after_idle > 0)
		{
			--cycles_after_idle;
			return;
		}

		reset();
		state = EXECUTING;
	}

	if(state == EXECUTING)
	{
		on_cycle();
		return;
	}

	// unreachable
	throw internal_error();
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

void base_unit::read_and_idle(std::uint32_t addr, std::uint8_t size, bus_manager::on_complete cb)
{
	read(addr, size, cb);
	go_idle = true;
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
		dec_addr(addr_reg, 2);

		this->cb = cb;
		this->reg_to_dec = addr_reg;
		
		auto on_read_lsw = [this](std::uint32_t data, size_type)
		{
			this->data = data;

			// we cannot dec in earler due to possible exceptions
			dec_addr(reg_to_dec, 2);
		};
		scheduler.read(regs.A(addr_reg).LW, size_type::WORD, on_read_lsw);

		auto on_read_msw = [this](std::uint32_t data, size_type)
		{
			// we read MSW
			this->data |= data << 16;
			if(this->cb) this->cb();
		};
		scheduler.read(regs.A(addr_reg).LW - 2, size_type::WORD, on_read_msw);

		state = SCHEDULER;

		return;
	}

	dec_addr(addr_reg, size);
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
	this->cb = cb;

	size_type sz;
	if(size == 1)
		sz = size_type::BYTE;
	else if(size == 2)
		sz = size_type::WORD;
	else
		sz = size_type::LONG;

	auto on_complete = [this](std::uint32_t data, size_type)
	{
		this->imm = data;
		if(this->cb) this->cb();
	};

	scheduler.read_imm(sz, on_complete);
	state = SCHEDULER;
}

void base_unit::read_imm_and_idle(std::uint8_t size, bus_manager::on_complete cb)
{
	read_imm(size, cb);
	go_idle = true;
}

void base_unit::write(std::uint32_t addr, std::uint32_t data, std::uint8_t size)
{
	if(size == 1)
		write_byte(addr, data);
	else if(size == 2)
		write_word(addr, data);
	else
		write_long(addr, data);
}

void base_unit::write_byte(std::uint32_t addr, std::uint8_t data)
{
	state = SCHEDULER;
	scheduler.write(addr, data, size_type::BYTE);
}

void base_unit::write_word(std::uint32_t addr, std::uint16_t data)
{
	state = SCHEDULER;
	scheduler.write(addr, data, size_type::WORD);
}

void base_unit::write_long(std::uint32_t addr, std::uint32_t data)
{
	state = SCHEDULER;
	scheduler.write(addr, data, size_type::LONG);
}

void base_unit::write_and_idle(std::uint32_t addr, std::uint32_t data, std::uint8_t size)
{
	if(size == 1)
		write_byte_and_idle(addr, data);
	else if(size == 2)
		write_word_and_idle(addr, data);
	else
		write_long_and_idle(addr, data);
}

void base_unit::write_byte_and_idle(std::uint32_t addr, std::uint8_t data)
{
	write_byte(addr, data);
	go_idle = true;
}

void base_unit::write_word_and_idle(std::uint32_t addr, std::uint16_t data)
{
	write_word(addr, data);
	go_idle = true;
}

void base_unit::write_long_and_idle(std::uint32_t addr, std::uint32_t data)
{
	write_long(addr, data);
	go_idle = true;
}

void base_unit::prefetch_one()
{
	state = SCHEDULER;
	scheduler.prefetch_one();
}

void base_unit::prefetch_two()
{
	state = SCHEDULER;
	scheduler.prefetch_two();
}

void base_unit::prefetch_irc()
{
	state = SCHEDULER;
	scheduler.prefetch_irc();
}

void base_unit::prefetch_one_and_idle()
{
	prefetch_one();
	go_idle = true;
}

void base_unit::prefetch_two_and_idle()
{
	prefetch_two();
	go_idle = true;
}

void base_unit::prefetch_irc_and_idle()
{
	prefetch_irc();
	go_idle = true;
}

void base_unit::wait(std::uint8_t cycles)
{
	cycles_to_wait = cycles - 1; // assume current cycle is already spent
	state = WAITING;
}

void base_unit::wait_and_idle(std::uint8_t cycles)
{
	wait(cycles);
	go_idle = true;
}

void base_unit::wait_after_idle(std::uint8_t cycles)
{
	cycles_after_idle = cycles;
}

void base_unit::wait_scheduler()
{
	state = SCHEDULER;
}

void base_unit::wait_scheduler_and_idle()
{
	// state = WAIT_SCHEDULER;
	go_idle = true;
}

void base_unit::inc_addr(std::uint8_t reg, std::uint8_t size)
{
	if(reg == 0b111 && size == 1)
		size = 2;
	regs.A(reg).LW += size;
}

void base_unit::dec_addr(std::uint8_t reg, std::uint8_t size)
{
	if(reg == 0b111 && size == 1)
		size = 2;
	regs.A(reg).LW -= size;
}

}
