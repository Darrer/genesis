#ifndef __M68K_EXECUTIONER_HPP__
#define __M68K_EXECUTIONER_HPP__

#include <cstdint>
#include <iostream>

#include "prefetch_queue.hpp"
#include "ea_decoder.hpp"
#include "bus_manager.hpp"

namespace genesis::m68k
{

class executioner
{
private:
	enum exec_state : std::uint8_t
	{
		IDLE,
		START_EXEC,
		EXECUTING,
		WAIT_EA_DECODING,
		WAIT_MEM_READ,
		WAIT_MEM_WRITE,

		PREFETCHING,
		PREFETCHING_AND_IDLE,
	};

public:
	executioner(m68k::cpu& cpu, bus_manager& busm, ea_decoder& dec)
		: cpu(cpu), regs(cpu.registers()), busm(busm), pq(cpu.prefetch_queue()), ea_dec(dec)
	{
	}

	bool is_idle() const
	{
		if(ex_state == IDLE)
			return true;

		if(ex_state == PREFETCHING_AND_IDLE)
			return pq.is_idle(); // if prefetch is done we have nothing left to do

		if(ex_state == WAIT_MEM_WRITE)
			return busm.is_idle();

		return false;
	}

	void cycle()
	{
		switch (ex_state)
		{
		case IDLE:
			ex_state = START_EXEC;
			[[fallthrough]];
		
		case START_EXEC:
			exec_stage = 0;
			opcode = pq.IRD;
			regs.PC += 2;

			ex_state = EXECUTING;
			[[fallthrough]];

		case EXECUTING:
			execute();
			break;

		case WAIT_EA_DECODING:
			if(ea_dec.ready())
			{
				ex_state = EXECUTING;
				execute();
			}
			break;

		case WAIT_MEM_READ:
		case WAIT_MEM_WRITE:
		// TODO: here we lost 1 cycle
			if(busm.is_idle())
			{
				ex_state = IDLE;
				cycle(); // TODO: don't do recursion
			}
			break;

		/* prefetch */
		case PREFETCHING:
			if(pq.is_idle())
			{
				ex_state = EXECUTING;
				execute(); // TODO: don't call execute directly
			}
			break;

		case PREFETCHING_AND_IDLE:
			if(pq.is_idle())
			{
				ex_state = IDLE;
				cycle(); // TODO: don't do recursion
			}
			break;

		default:
			throw std::runtime_error("executioner::cycle internal error: unknown state");
		}
	}

private:
	void execute()
	{
		if((opcode >> 12) == 0b1101)
			exec_add();
		else if((opcode >> 8) == 0b110)
			exec_addi();
		else if((opcode >> 12) == 0b0101)
			exec_addq();
		else
			throw std::runtime_error("execute: not implemented yet " + std::to_string(opcode));
	}

private:
	void decode_ea(std::uint8_t ea, std::size_t size = 1)
	{
		ea_dec.decode(ea, size);
		if(ea_dec.ready())
		{
			// immediate decoding
			ex_state = EXECUTING;
			// TODO: need to repeat command
			execute();
		}
		else
		{
			// wait till decoding is over
			ex_state = WAIT_EA_DECODING;
		}
	}

	void read_byte(std::uint32_t addr)
	{
		busm.init_read_byte(addr);
		ex_state = WAIT_MEM_READ;
	}

	void read_word(std::uint32_t addr)
	{
		busm.init_read_word(addr);
		ex_state = WAIT_MEM_READ;
	}

	template<class T>
	void write_and_idle(std::uint32_t addr, T val)
	{
		busm.init_write(addr, val);
		ex_state = WAIT_MEM_WRITE;
	}

	void prefetch_and_idle()
	{
		pq.init_fetch_one();
		ex_state = PREFETCHING_AND_IDLE;
	}

	void prefetch()
	{
		pq.init_fetch_one();
		ex_state = PREFETCHING;
	}

	void read_imm(/*std::size_t size = 1*/)
	{
		// TODO: support 2/4 size
		imm = pq.IRC & 0xFF;
		pq.init_fetch_irc();
		regs.PC += 2;
		ex_state = PREFETCHING;
	}

private:
	void exec_add()
	{
		// std::cout << "exec_add" << std::endl;
		switch (exec_stage++)
		{
		case 0:
			decode_ea(pq.IRD & 0xFF, 1);
			break;

		case 1:
		{
			auto& reg = regs.D((opcode >> 9) & 0x7).B;

			auto op = ea_dec.result();
			if(op.has_pointer())
			{
				res = add(reg, op.pointer().value<std::uint8_t>());
			}
			else if(op.has_data_reg())
			{
				res = add(reg, op.data_reg().B);
			}
			else
			{
				throw std::runtime_error("internal error add");
			}

			std::uint8_t opmode = (opcode >> 6) & 0x7;
			if(opmode == 0b000)
			{
				// std::cout << "Writing result to register" << std::endl;
				reg = res;
				prefetch_and_idle();
			}
			else
			{
				prefetch();
			}

			break;
		}

		case 2:
			// std::cout << "Going to write " << (int)res << ", at " << (op.pointer().address & 0xFFFFFF) << std::endl;
			write_and_idle(ea_dec.result().pointer().address, res);
			break;

		default:
			throw std::runtime_error("executioner::exec_add internal error: unknown state");
		}
	}

	void exec_addi()
	{
		switch (exec_stage++)
		{
		case 0:
			read_imm();
			break;
		
		case 1:
			decode_ea(pq.IRD & 0xFF, 1);
			break;

		case 2:
		{
			auto op = ea_dec.result();
			if(op.has_pointer())
			{
				res = add((std::uint8_t)imm, op.pointer().value<std::uint8_t>());
				prefetch();
			}
			else if(op.has_data_reg())
			{
				op.data_reg().B = add((std::uint8_t)imm, op.data_reg().B);
				prefetch_and_idle();
			}
			else
			{
				throw std::runtime_error("internal error addi");
			}

			break;
		}

		case 3:
			write_and_idle(ea_dec.result().pointer().address, res);
			break;

		default:
			throw std::runtime_error("executioner::exec_addi internal error: unknown state");
		}
	}

	void exec_addq()
	{
		switch (exec_stage++)
		{
		case 0:
			decode_ea(pq.IRD & 0xFF, 1);
			break;

		case 1:
		{
			std::uint8_t data = (opcode >> 9) & 0x7;
			if(data == 0) data = 8;

			auto op = ea_dec.result();
			if(op.has_pointer())
			{
				res = add(data, op.pointer().value<std::uint8_t>());
				prefetch();
			}
			else if(op.has_data_reg())
			{
				op.data_reg().B = add(data, op.data_reg().B);
				prefetch_and_idle();
			}
			else
			{
				throw std::runtime_error("internal error addi");
			}

			break;
		}

		case 2:
			write_and_idle(ea_dec.result().pointer().address, res);
			break;

		default:
			throw std::runtime_error("executioner::exec_addq internal error: unknown state");
		}
	}

private:
	// TODO: move away
	template<class T>
	T add(T a, T b)
	{
		// std::cout << "Add " << (int)a << " + " << (int)b << std::endl;
		return a + b;
	}

private:
	m68k::cpu& cpu;

	cpu_registers& regs;
	bus_manager& busm;
	prefetch_queue& pq;
	ea_decoder& ea_dec;

	exec_state ex_state = IDLE;
	std::uint16_t opcode = 0;
	std::uint8_t res = 0;
	std::uint8_t exec_stage;

	std::uint32_t imm;
};

}

#endif // __M68K_EXECUTIONER_HPP__
