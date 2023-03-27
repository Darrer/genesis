#include "base_unit.h"

#include "exception.hpp"

#include <iostream>

enum handler_state : std::uint8_t
{
	IDLE,
	EXECUTING,
	WAITING_RW,
	PREFETCHING,
	WAITING,
};

namespace genesis::m68k
{

base_unit::base_unit(m68k::cpu_registers& regs, m68k::bus_manager& busm, m68k::prefetch_queue& pq):
	regs(regs), busm(busm), pq(pq)
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

	if(state == WAITING_RW && go_idle)
		return busm.is_idle();

	if(state == PREFETCHING && go_idle)
		return pq.is_idle();

	if(state == WAITING && go_idle)
		return cycles_to_wait == 0;

	return state == IDLE;
}

void base_unit::cycle()
{
	if(state == WAITING_RW)
	{
		if(!busm.is_idle())
			return;

		state = go_idle ? IDLE : EXECUTING;
	}

	if(state == PREFETCHING)
	{
		if(!pq.is_idle())
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

		// TODO: due to this chaining we occupy bus for 8 cycles
		auto on_read_msw = [this, cb]()
		{
			// we read MSW
			data |= busm.letched_word() << 16;
			if(cb) cb();
		};

		auto on_read_lsw = [this, addr_reg, on_read_msw]()
		{
			data = busm.letched_word();

			// dec
			dec_addr(addr_reg, 2);

			// read LSW
			busm.init_read_word(regs.A(addr_reg).LW, addr_space::DATA, on_read_msw);
		};

		busm.init_read_word(regs.A(addr_reg).LW, addr_space::DATA, on_read_lsw);
		state = WAITING_RW;

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
	auto on_complete = [this, cb]()
	{
		data = busm.letched_byte();
		if(cb) cb();
	};

	busm.init_read_byte(addr, addr_space::DATA, on_complete);
	state = WAITING_RW;
}

void base_unit::read_word(std::uint32_t addr, bus_manager::on_complete cb)
{
	auto on_complete = [this, cb]()
	{
		data = busm.letched_word();
		if(cb) cb();
	};

	busm.init_read_word(addr, addr_space::DATA, on_complete);
	state = WAITING_RW;
}

void base_unit::read_long(std::uint32_t addr, bus_manager::on_complete cb)
{
	// TODO: due to this chaining we occupy bus for 8 cycles
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
		busm.init_read_word(addr + 2, addr_space::DATA, on_read_lsw);
	};

	busm.init_read_word(addr, addr_space::DATA, on_read_msw);
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

		busm.init_read_word(regs.PC, addr_space::PROGRAM, on_complete);
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

void base_unit::read_imm_and_idle(std::uint8_t size, bus_manager::on_complete cb)
{
	read_imm(size, cb);
	go_idle = true;
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
