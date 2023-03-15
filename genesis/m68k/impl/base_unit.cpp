#include "base_unit.h"

#include "exception.hpp"

#include <iostream>

enum handler_state : std::uint8_t
{
	IDLE,
	WAITING_RW,
	PREFETCHING,
	WAITING,
};

namespace genesis::m68k
{

base_unit::base_unit(m68k::cpu_registers& regs, m68k::bus_manager& busm, m68k::prefetch_queue& pq):
	regs(regs), busm(busm), pq(pq)
{
	reset();
}

void base_unit::reset()
{
	state = IDLE;
	imm = 0;
	data = 0;
	cycles_to_wait = 0;
	cycles_after_idle = 0;
	need_wait = false;
}

bool base_unit::is_idle() const
{
	if(state != IDLE && cycles_after_idle != 0)
		return false;

	if(state == WAITING_RW)
		return busm.is_idle();

	if(state == PREFETCHING)
		return pq.is_idle();

	if(state == WAITING)
		return cycles_to_wait == 0;

	if(state == IDLE)
		return cycles_after_idle == 0;

	return false;
}

void base_unit::cycle()
{
	switch (state)
	{
	case IDLE:
		call_on_cycle();
		break;

	case WAITING_RW:
		if(busm.is_idle())
		{
			state = IDLE;
			call_on_cycle();
		}
		break;

	case PREFETCHING:
		if(pq.is_idle())
		{
			state = IDLE;
			call_on_cycle();
		}
		break;

	case WAITING:
		if(--cycles_to_wait == 0)
			state = IDLE;
		break;

	default:
		throw internal_error();
	}

}

void base_unit::call_on_cycle()
{
	if(state == IDLE && need_wait && cycles_after_idle > 0)
	{
		--cycles_after_idle;
		if(cycles_after_idle == 0)
			need_wait = false;
	}
	else
	{
		on_cycle();
	}
}

void base_unit::read_and_idle(std::uint32_t addr, std::uint8_t size, bus_manager::on_complete cb)
{
	if(size == 1)
		read_byte(addr, cb);
	else if(size == 2)
		read_word(addr, cb);
	else
		read_long(addr, cb);

	set_idle();
	need_wait = true;
}

void base_unit::read_byte(std::uint32_t addr, bus_manager::on_complete cb)
{
	auto on_complete = [this, cb]()
	{
		data = busm.letched_byte();
		if(cb) cb();
	};

	busm.init_read_byte(addr, on_complete);
	state = WAITING_RW;
}

void base_unit::read_word(std::uint32_t addr, bus_manager::on_complete cb)
{
	auto on_complete = [this, cb]()
	{
		data = busm.letched_word();
		if(cb) cb();
	};

	busm.init_read_word(addr, on_complete);
	state = WAITING_RW;
}

void base_unit::read_long(std::uint32_t addr, bus_manager::on_complete cb)
{
	auto on_read_lsw = [this, cb]()
	{
		data = data | busm.letched_word();
		if(cb) cb();
	};

	auto on_read_msw = [this, addr, on_read_lsw]()
	{
		// we read MSW
		data = busm.letched_word();

		// assume it's little-endian
		data = data << 16;

		// read LSW
		busm.init_read_word(addr + 2, on_read_lsw);
	};

	busm.init_read_word(addr, on_read_msw);
	state = WAITING_RW;
}

void base_unit::read_imm(std::uint8_t size, bus_manager::on_complete cb)
{
	if(size == 4)
	{
		imm = (std::uint32_t)pq.IRC << 16;
		regs.PC += 2;

		auto on_complete = [this, cb]()
		{
			imm = imm | busm.letched_word();
			if(cb) cb(); // data is available, good to cb
			prefetch_irc();
		};

		busm.init_read_word(regs.PC, on_complete);
		state = WAITING_RW;
	}
	else
	{
		if(size == 1)
			imm = pq.IRC & 0xFF;
		else // size == 2
			imm = pq.IRC;
		
		if(cb) cb();
		prefetch_irc();
	}
}

void base_unit::write_byte(std::uint32_t addr, std::uint8_t data)
{
	busm.init_write(addr, data);
	state = WAITING_RW;
}

void base_unit::write_word(std::uint32_t addr, std::uint16_t data)
{
	busm.init_write(addr, data);
	state = WAITING_RW;
}

void base_unit::write_long(std::uint32_t addr, std::uint32_t data)
{
	auto write_msw = [this, addr, data]()
	{
		std::uint16_t msw = data >> 16;
		busm.init_write(addr, msw);
	};

	std::uint16_t lsw = data & 0xFFFF;
	busm.init_write(addr + 2, lsw, write_msw);
	state = WAITING_RW;
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
	busm.init_write(addr, data);
	state = WAITING_RW;
	set_idle();
	need_wait = true;
}

void base_unit::write_word_and_idle(std::uint32_t addr, std::uint16_t data)
{
	busm.init_write(addr, data);
	state = WAITING_RW;
	set_idle();
	need_wait = true;
}

void base_unit::write_long_and_idle(std::uint32_t addr, std::uint32_t data)
{
	write_long(addr, data);
	set_idle();
	need_wait = true;
}

void base_unit::prefetch_one()
{
	pq.init_fetch_one();
	state = PREFETCHING;
}

void base_unit::prefetch_two()
{
	pq.init_fetch_two();
	state = PREFETCHING;
}

void base_unit::prefetch_irc()
{
	pq.init_fetch_irc();
	regs.PC += 2;
	state = PREFETCHING;
}

void base_unit::prefetch_one_and_idle()
{
	pq.init_fetch_one();
	state = PREFETCHING;
	set_idle();
	need_wait = true;
}

void base_unit::prefetch_two_and_idle()
{
	pq.init_fetch_two();
	state = PREFETCHING;
	set_idle();
	need_wait = true;
}

void base_unit::prefetch_irc_and_idle()
{
	prefetch_irc();
	set_idle();
	need_wait = true;
}

void base_unit::wait(std::uint8_t cycles)
{
	cycles_to_wait = cycles;
	state = WAITING;
}

void base_unit::wait_and_idle(std::uint8_t cycles)
{
	if(cycles <= 1)
		throw internal_error();

	cycles_to_wait = cycles - 1; // assume the current cycle is already spent
	state = WAITING;
	set_idle();
	need_wait = true;
}

void base_unit::wait_after_idle(std::uint8_t cycles)
{
	need_wait = false;
	cycles_after_idle = cycles;
}

}
