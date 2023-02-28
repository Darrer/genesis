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

		PREFETCH_ONE,
		WAIT_PREFETCH_ONE,
	};

public:
	executioner(m68k::cpu& cpu, bus_manager& busm, ea_decoder& dec)
		: cpu(cpu), regs(cpu.registers()), busm(busm), pq(cpu.prefetch_queue()), ea_dec(dec)
	{
	}

	bool is_idle() const { return ex_state == IDLE; }

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
				ex_state = IDLE;
			break;

		case PREFETCH_ONE:
			pq.init_fetch_one();
			ex_state = WAIT_PREFETCH_ONE;
			break;

		case WAIT_PREFETCH_ONE:
			if(pq.is_idle())
			{
				ex_state = EXECUTING;
				execute();
			}
			break;

		default:
			throw std::runtime_error("executioner::cycle internal error: unknown state");
		}
	}

private:
	void execute()
	{
		if((opcode >> 12) != 0b1101)
			throw std::runtime_error("execute: not implemented yet");

		exec_add();
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

	void prefetch_one()
	{
		pq.init_fetch_one();
		ex_state = WAIT_PREFETCH_ONE;
	}

private:
	void exec_add()
	{
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
				std::cout << "Writing result to register" << std::endl;
				reg = res;
				ex_state = IDLE;
			}
			else
			{
				prefetch_one();
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

private:
	// TODO: move away
	template<class T>
	T add(T a, T b)
	{
		std::cout << "Add " << (int)a << " + " << (int)b << std::endl;
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
};

}

#endif // __M68K_EXECUTIONER_HPP__
