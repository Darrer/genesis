#include "base_handler.h"

#include "exception.hpp"


enum handler_state : std::uint8_t
{
	IDLE,
	READING,
	WRITING,
	WRITING_AND_IDLE,
	PREFETCHING,
	PREFETCHING_AND_IDLE,
	WAITING,
	WAITING_AND_IDLE,
};

namespace genesis::m68k
{

base_handler::base_handler(m68k::cpu_registers& regs, m68k::bus_manager& busm, m68k::prefetch_queue& pq):
	regs(regs), busm(busm), pq(pq)
{
	reset();
}

void base_handler::reset()
{
	state = IDLE;
	imm = 0;
	cycles_to_wait = 0;
}

bool base_handler::is_idle() const
{
	if(state == IDLE)
		return true;

	if(state == WRITING_AND_IDLE)
		return busm.is_idle();

	if(state == PREFETCHING_AND_IDLE)
		return pq.is_idle();

	if(state == WAITING_AND_IDLE)
		return cycles_to_wait == 0;

	return false;
}

void base_handler::cycle()
{
	switch (state)
	{
	case IDLE:
		on_cycle();
		break;

	case READING:
	case WRITING:
	case WRITING_AND_IDLE:
		if(busm.is_idle())
		{
			state = IDLE;
			on_cycle();
		}
		break;

	case PREFETCHING:
	case PREFETCHING_AND_IDLE:
		if(pq.is_idle())
		{
			state = IDLE;
			on_cycle();
		}
		break;

	case WAITING:
	case WAITING_AND_IDLE:
		if(cycles_to_wait == 0)
		{
			state = IDLE;
			on_cycle();
		}
		else
		{
			--cycles_to_wait;
		}
		break;

	default:
		throw internal_error();
	}

}

void base_handler::read_byte(std::uint32_t addr)
{
	busm.init_read_byte(addr);
	state = READING;
}

void base_handler::read_word(std::uint32_t addr)
{
	busm.init_read_word(addr);
	state = READING;
}

void base_handler::read_long(std::uint32_t /*addr*/)
{
	throw not_implemented();
}

void base_handler::write_byte(std::uint32_t addr, std::uint8_t data)
{
	busm.init_write(addr, data);
	state = WRITING;
}

void base_handler::write_word(std::uint32_t addr, std::uint16_t data)
{
	busm.init_write(addr, data);
	state = WRITING;
}

void base_handler::write_long(std::uint32_t /*addr*/, std::uint32_t /*data*/)
{
	throw not_implemented();
}

void base_handler::write_and_idle(std::uint32_t addr, std::uint32_t data, std::uint8_t size)
{
	if(size == 1)
		write_byte_and_idle(addr, data);
	else if(size == 2)
		write_word_and_idle(addr, data);
	else
		write_long_and_idle(addr, data);
}

void base_handler::write_byte_and_idle(std::uint32_t addr, std::uint8_t data)
{
	busm.init_write(addr, data);
	state = WRITING_AND_IDLE;
	set_idle();
}

void base_handler::write_word_and_idle(std::uint32_t addr, std::uint16_t data)
{
	busm.init_write(addr, data);
	state = WRITING_AND_IDLE;
	set_idle();
}

void base_handler::write_long_and_idle(std::uint32_t /*addr*/, std::uint32_t /*data*/)
{
	throw not_implemented();
}

void base_handler::prefetch()
{
	pq.init_fetch_one();
	state = PREFETCHING;
}

void base_handler::prefetch_and_idle()
{
	pq.init_fetch_one();
	state = PREFETCHING_AND_IDLE;
	set_idle();
}

void base_handler::read_imm(std::uint8_t size)
{
	if(size == 1)
		imm = pq.IRC & 0xFF;
	else if(size == 2)
		imm = pq.IRC;
	else
		throw not_implemented();

	pq.init_fetch_irc();
	state = PREFETCHING;
	regs.PC += 2;
}

void base_handler::wait(std::uint8_t cycles)
{
	cycles_to_wait = cycles;
	state = WAITING;
}

void base_handler::wait_and_idle(std::uint8_t cycles)
{
	cycles_to_wait = cycles;
	state = WAITING_AND_IDLE;
	set_idle();
}

}
