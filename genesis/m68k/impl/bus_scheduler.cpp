#include "bus_scheduler.h"

#include <iostream>

#include "exception.hpp"

namespace genesis::m68k
{

bus_scheduler::bus_scheduler(m68k::cpu_registers& regs, m68k::bus_manager& busm, m68k::prefetch_queue& pq):
	regs(regs), busm(busm), pq(pq)
{
}

void bus_scheduler::reset()
{
	current_op.reset();
	while(!queue.empty())
		queue.pop();
}

bool bus_scheduler::is_idle() const
{
	return queue.empty() && current_op_is_over();
}

bool bus_scheduler::current_op_is_over() const
{
	if(!current_op.has_value())
		return true;

	auto curr_type = current_op.value().type;

	switch (curr_type)
	{
	case op_type::READ:
	case op_type::READ_IMM:
	case op_type::WRITE:
		return busm.is_idle();
	
	case op_type::PREFETCH_IRC:
	case op_type::PREFETCH_ONE:
	case op_type::PREFETCH_TWO:
		return pq.is_idle();

	case op_type::WAIT:
		return curr_wait_cycles == 0;

	default: throw internal_error();
	}
}

void bus_scheduler::cycle()
{
	if(!current_op_is_over())
	{
		if(current_op.has_value() && current_op.value().type == op_type::WAIT)
			--curr_wait_cycles;

		return;
	}
	
	if(queue.empty())
		return;

	start_operation(queue.front());
	queue.pop();
}

void bus_scheduler::read_impl(std::uint32_t addr, size_type size, on_read_complete on_complete)
{
	if(size == size_type::BYTE || size == size_type::WORD)
	{
		read_operation read { addr, size, on_complete };
		queue.emplace(op_type::READ, read);
	}
	else
	{
		// m68k bus does not support long read/write, so
		// split to two word operations

		read_operation read_msw { addr, size, nullptr }; // call back only when second word is read
		read_operation read_lsw { addr + 2, size, on_complete};

		queue.emplace(op_type::READ, read_msw);
		queue.emplace(op_type::READ, read_lsw);
	}
}

void bus_scheduler::read_imm_impl(size_type size, on_read_complete on_complete)
{
	// we don't know the address yet, so schedule as is
	read_imm_operation read { size, on_complete };
	queue.emplace(op_type::READ_IMM, read);
}

void bus_scheduler::on_read_finish()
{
	if(!current_op.has_value())
		throw internal_error();
	
	auto curr_type = current_op.value().type;
	if(curr_type != op_type::READ && curr_type != op_type::READ_IMM)
		throw internal_error();

	auto op = current_op.value().op;

	size_type size;
	on_read_complete on_complete;
	if(curr_type == op_type::READ)
	{
		size = std::get<read_operation>(op).size;
		on_complete = std::get<read_operation>(op).on_complete;
	}
	else
	{
		size = std::get<read_imm_operation>(op).size;
		on_complete = std::get<read_imm_operation>(op).on_complete;
	}
	
	if(size == size_type::BYTE)
		data = busm.letched_byte();
	else if(size == size_type::WORD)
		data = busm.letched_word();
	else
		data = (data << 16) | busm.letched_word();

	if(on_complete != nullptr)
	{
		on_complete(data, size);
	}

	current_op.reset();
}

void bus_scheduler::write(std::uint32_t addr, std::uint32_t data, size_type size)
{
	if(size == size_type::BYTE || size == size_type::WORD)
	{
		write_operation write { addr, data, size };
		queue.emplace(op_type::WRITE, write);
	}
	else
	{
		// m68k bus does not support long read/write, so
		// split to two word operations

		write_operation write_lsw { addr + 2, data & 0xFFFF, size_type::WORD };
		write_operation write_msw { addr, data >> 16, size_type::WORD };

		queue.emplace(op_type::WRITE, write_lsw);
		queue.emplace(op_type::WRITE, write_msw);
	}
}

void bus_scheduler::prefetch_one()
{
	queue.push({ op_type::PREFETCH_ONE });
}

void bus_scheduler::prefetch_two()
{
	queue.push({ op_type::PREFETCH_TWO });
}

void bus_scheduler::prefetch_irc()
{
	queue.push({ op_type::PREFETCH_IRC });
}

void bus_scheduler::wait(std::uint16_t cycles)
{
	if(cycles == 0) return;

	wait_operation wait_op {cycles};
	queue.emplace(op_type::WAIT, wait_op);
}

void bus_scheduler::start_operation(operation op)
{
	current_op = op;
	switch (op.type)
	{
	case op_type::READ:
	{
		read_operation read = std::get<read_operation>(op.op);
		if(read.size == size_type::BYTE)
			busm.init_read_byte(read.addr, addr_space::DATA, [this]() { on_read_finish(); });
		else
			busm.init_read_word(read.addr, addr_space::DATA, [this]() { on_read_finish(); });

		break;
	}

	case op_type::READ_IMM:
	{
		read_imm_operation read = std::get<read_imm_operation>(op.op);
		if(read.size == size_type::BYTE || read.size == size_type::WORD)
		{
			std::uint32_t data = read.size == size_type::WORD ? pq.IRC : pq.IRC & 0xFF;
			if(read.on_complete)
			{
				read.on_complete(data, read.size);
			}

			start_operation({ op_type::PREFETCH_IRC });
		}
		else
		{
			data = pq.IRC;

			auto on_read_complete = [this]()
			{
				on_read_finish();
				start_operation({ op_type::PREFETCH_IRC });
			};

			regs.PC += 2;
			busm.init_read_word(regs.PC, addr_space::PROGRAM, on_read_complete);
		}

		break;
	}

	case op_type::WRITE:
	{
		write_operation write = std::get<write_operation>(op.op);
		if(write.size == size_type::BYTE)
			busm.init_write<std::uint8_t>(write.addr, write.data);
		else
			busm.init_write<std::uint16_t>(write.addr, write.data);

		break;
	}

	case op_type::PREFETCH_ONE:
		pq.init_fetch_one();
		break;

	case op_type::PREFETCH_IRC:
		pq.init_fetch_irc();
		regs.PC += 2;
		break;

	case op_type::PREFETCH_TWO:
		pq.init_fetch_two();
		break;

	case op_type::WAIT:
	{
		wait_operation wait = std::get<wait_operation>(op.op);
		curr_wait_cycles = wait.cycles - 1; // took current cycle
		break;
	}

	default: throw internal_error();
	}
}

}
