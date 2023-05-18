#include "bus_scheduler.h"
#include "exception.hpp"


namespace genesis::m68k
{

bus_scheduler::bus_scheduler(m68k::cpu_registers& regs, m68k::bus_manager& busm):
	regs(regs), busm(busm), pq(busm, regs)
{
}

void bus_scheduler::reset()
{
	current_op.reset();
	while(!queue.empty())
		queue.pop();
	pq.reset();
}

bool bus_scheduler::is_idle() const
{
	return queue.empty() && current_op_is_over();
}

bool bus_scheduler::current_op_is_over() const
{
	return !current_op.has_value();
}

void bus_scheduler::cycle()
{
	if(!current_op_is_over())
	{
		if(curr_wait_cycles > 0)
		{
			--curr_wait_cycles;
			if(curr_wait_cycles == 0)
				run_imm_operations();
		}

		return;
	}

	if(queue.empty())
		return;

	run_imm_operations();

	start_operation(queue.front());
	queue.pop();
}

void bus_scheduler::read_impl(std::uint32_t addr, size_type size, addr_space space, on_read_complete on_complete)
{
	if(size == size_type::BYTE || size == size_type::WORD)
	{
		read_operation read { addr, size, space, on_complete };
		queue.emplace(op_type::READ, read);

		// current_op = { op_type::READ, read };
		// busm.init_read_word(read.addr, addr_space::DATA, [this]() { on_read_finished(); });
		// busm.init_read_word(addr, addr_space::DATA, [this]() { on_read_finished(); });

		// operation op{ op_type::READ, read };
		// start_operation(op);
	}
	else
	{
		read_operation read_msw { addr, size, space, nullptr }; // call back only when second word is read
		read_operation read_lsw { addr + 2, size, space, on_complete};

		queue.emplace(op_type::READ, read_msw);
		queue.emplace(op_type::READ, read_lsw);
	}
}

void bus_scheduler::read_imm_impl(size_type size, on_read_complete on_complete, read_imm_flags flags)
{
	if(size == size_type::BYTE || size == size_type::WORD)
	{
		// we don't know the address yet, so schedule as is
		read_imm_operation read { size, on_complete, flags };
		queue.emplace(op_type::READ_IMM, read);
	}
	else
	{
		if(flags == read_imm_flags::do_prefetch)
		{
			read_imm_operation read_msw { size, nullptr, flags };
			read_imm_operation read_lsw { size, on_complete, flags };

			queue.emplace(op_type::READ_IMM, read_msw);
			queue.emplace(op_type::READ_IMM, read_lsw);
		}
		else
		{
			read_imm_operation read { size, on_complete, flags };
			queue.emplace(op_type::READ_IMM, read);
		}
	}
}

void bus_scheduler::latch_data(size_type size)
{
	if(size == size_type::BYTE)
		data = busm.letched_byte();
	else if(size == size_type::WORD)
		data = busm.letched_word();
	else
		data = (data << 16) | busm.letched_word();
}

void bus_scheduler::on_read_finished()
{
	read_operation& read = std::get<read_operation>(current_op.value().op);

	latch_data(read.size);

	if(read.on_complete != nullptr)
		read.on_complete(data, read.size);

	current_op.reset();
	run_imm_operations();
}

void bus_scheduler::on_read_imm_finished()
{
	read_imm_operation& imm = std::get<read_imm_operation>(current_op.value().op);

	latch_data(imm.size);

	if(imm.on_complete != nullptr)
		imm.on_complete(data, imm.size);

	current_op.reset();
	run_imm_operations();
}

void bus_scheduler::write(std::uint32_t addr, std::uint32_t data, size_type size, order order)
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

		if(order == order::lsw_first)
		{
			queue.emplace(op_type::WRITE, write_lsw);
			queue.emplace(op_type::WRITE, write_msw);
		}
		else
		{
			queue.emplace(op_type::WRITE, write_msw);
			queue.emplace(op_type::WRITE, write_lsw);
		}
	}
}

void bus_scheduler::prefetch_ird()
{
	queue.push({ op_type::PREFETCH_IRD });
}

void bus_scheduler::prefetch_irc()
{
	queue.push({ op_type::PREFETCH_IRC });
}

void bus_scheduler::prefetch_one()
{
	queue.push({ op_type::PREFETCH_ONE });
}

void bus_scheduler::prefetch_two()
{
	prefetch_ird();
	prefetch_irc();
}

void bus_scheduler::wait(std::uint16_t cycles)
{
	if(cycles == 0) return;

	wait_operation wait_op {cycles};
	queue.emplace(op_type::WAIT, wait_op);
}

void bus_scheduler::call_impl(callback cb)
{
	call_operation call_op { cb };
	queue.emplace(op_type::CALL, call_op);
}

void bus_scheduler::inc_addr_reg(std::uint8_t reg, size_type size)
{
	register_operation reg_op { reg, size };
	queue.emplace(op_type::INC_ADDR, reg_op);
}

void bus_scheduler::dec_addr_reg(std::uint8_t reg, size_type size)
{
	register_operation reg_op { reg, size };
	queue.emplace(op_type::DEC_ADDR, reg_op);
}

void bus_scheduler::push(std::uint32_t data, size_type size, order order)
{
	if(size == size_type::LONG)
	{
		push_operation push_lsw { data & 0xFFFF, size_type::WORD };
		push_operation push_msw { data >> 16, size_type::WORD };

		if(order == order::lsw_first)
		{
			queue.emplace(op_type::PUSH, push_lsw);
			queue.emplace(op_type::PUSH, push_msw);
		}
		else
		{
			push_msw.offset = -2;
			push_lsw.offset = 2;
			queue.emplace(op_type::PUSH, push_msw);
			queue.emplace(op_type::PUSH, push_lsw);
		}
	}
	else
	{
		push_operation push { data, size };
		queue.emplace(op_type::PUSH, push);
	}
}

void bus_scheduler::start_operation(operation& op)
{
	current_op = op;
	switch (op.type)
	{
	case op_type::READ:
	{
		read_operation read = std::get<read_operation>(op.op);
		if(read.size == size_type::BYTE)
			busm.init_read_byte(read.addr, read.space, [this]() { on_read_finished(); });
		else
			busm.init_read_word(read.addr, read.space, [this]() { on_read_finished(); });

		break;
	}

	case op_type::READ_IMM:
	{
		read_imm_operation read = std::get<read_imm_operation>(op.op);
		if(read.size == size_type::BYTE || read.size == size_type::WORD)
			data = read.size == size_type::BYTE ? regs.IRC & 0xFF : regs.IRC;
		else
			data = (data << 16) | regs.IRC;

		if(read.flags == read_imm_flags::do_prefetch)
		{
			current_op = { op_type::PREFETCH_IRC };
			pq.init_fetch_irc([this]() { run_imm_operations(); });
			regs.PC += 2;
		}
		// Even if we're requested to not do a prefetch, we must read second word for a long operation
		else if(read.size == size_type::LONG)
		{
			busm.init_read_word(regs.PC + 2, addr_space::PROGRAM, [this]() { on_read_imm_finished(); });
			return;
		}
		else
		{
			// Read imm with no_prefetch flag for byte/word are cycle-free, if got here - we have a cycle issue
			throw internal_error();
		}

		if(read.on_complete)
			read.on_complete(data, read.size);

		break;
	}

	case op_type::WRITE:
	{
		// TODO: call back when write is finished?
		write_operation write = std::get<write_operation>(op.op);
		if(write.size == size_type::BYTE)
			busm.init_write<std::uint8_t>(write.addr, write.data, [this]() { run_imm_operations(); });
		else
			busm.init_write<std::uint16_t>(write.addr, write.data, [this]() { run_imm_operations(); });

		break;
	}

	case op_type::PUSH:
	{
		push_operation push = std::get<push_operation>(op.op);

		regs.dec_addr(7, push.size);

		if(push.size == size_type::BYTE)
			busm.init_write<std::uint8_t>(regs.SP().LW + push.offset, push.data, [this]() { run_imm_operations(); });
		else
			busm.init_write<std::uint16_t>(regs.SP().LW + push.offset, push.data, [this]() { run_imm_operations(); });

		break;
	}

	case op_type::PREFETCH_IRD:
		pq.init_fetch_ird([this]() { run_imm_operations(); });
		break;

	case op_type::PREFETCH_IRC:
		pq.init_fetch_irc([this]() { run_imm_operations(); });
		break;

	case op_type::PREFETCH_ONE:
		pq.init_fetch_one([this]() { run_imm_operations(); });
		break;

	case op_type::WAIT:
	{
		wait_operation wait = std::get<wait_operation>(op.op);
		curr_wait_cycles = wait.cycles - 1; // took current cycle
		if(curr_wait_cycles == 0)
			run_imm_operations();
		break;
	}

	default: throw internal_error();
	}
}

void bus_scheduler::run_imm_operations()
{
	current_op.reset();

	while(!queue.empty())
	{
		auto type = queue.front().type;
		switch (type)
		{
		case op_type::CALL:
		{
			auto call_op = std::get<call_operation>(queue.front().op);
			call_op.cb();
			break;
		}

		case op_type::INC_ADDR:
		case op_type::DEC_ADDR:
		{
			auto reg_op = std::get<register_operation>(queue.front().op);
			if(type == op_type::INC_ADDR)
				regs.inc_addr(reg_op.reg, reg_op.size);
			else
				regs.dec_addr(reg_op.reg, reg_op.size);

			break;
		}

		default:
			return;
		}

		queue.pop();
	}
}

}
