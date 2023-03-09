#ifndef __M68K_EXECUTIONER_HPP__
#define __M68K_EXECUTIONER_HPP__

#include <cstdint>
#include <iostream>

#include "prefetch_queue.hpp"
#include "ea_decoder.hpp"
#include "bus_manager.hpp"

namespace genesis::m68k
{

#define throw_invalid_opcode() \
	throw std::runtime_error(std::string("executioner::") + __func__ + " error: invalid opcode")

#define throw_internal_error() \
	throw std::runtime_error(std::string("executioner::") + __func__ + " internal error")

#define throw_not_implemented_yet() \
	throw std::runtime_error(std::string("executioner::") + __func__ + " error: not implemented yet")

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

	void read_imm(std::uint8_t size)
	{
		if(size == 1)
			imm = pq.IRC & 0xFF;
		else if(size == 2)
			imm = pq.IRC;
		else if(size == 4)
			throw_not_implemented_yet();
		else
			throw_internal_error();

		pq.init_fetch_irc();
		regs.PC += 2;
		ex_state = PREFETCHING;
	}

private:
	void exec_add()
	{
		std::cout << "exec_add" << std::endl;

		const std::uint8_t opmode = (opcode >> 6) & 0x7;
		std::uint8_t size = 0;
		if(opmode == 0b000 || opmode == 0b100)
			size = 1;
		else if(opmode == 0b001 || opmode == 0b101)
			size = 2;
		else if(opmode == 0b010 || opmode == 0b110)
			size = 4;
		else
			throw_invalid_opcode();

		switch (exec_stage++)
		{
		case 0:
			decode_ea(pq.IRD & 0xFF, size);
			break;

		case 1:
		{
			auto& reg = regs.D((opcode >> 9) & 0x7);
			auto op = ea_dec.result();

			switch (opmode)
			{
			case 0b000:
			case 0b100:
				if(op.has_pointer())
					res = add(reg.B, op.pointer().value<std::uint8_t>());
				else if(op.has_data_reg())
					res = add(reg.B, op.data_reg().B);
				else
					throw_invalid_opcode();

				if(opmode == 0b000)
				{
					reg.B = (std::uint8_t)res;
					prefetch_and_idle();
				}
				else
				{
					prefetch();
				}
				break;

			case 0b001:
			case 0b101:
				if(op.has_pointer())
					res = add(reg.W, op.pointer().value<std::uint16_t>());
				else if(op.has_data_reg())
					res = add(reg.W, op.data_reg().W);
				else if(op.has_addr_reg())
					res = add(reg.W, op.addr_reg().W);
				else
					throw_internal_error();
				
				if(opmode == 0b001)
				{
					reg.W = (std::uint16_t)res;
					prefetch_and_idle();
				}
				else
				{
					prefetch();
				}

				break;
			
			case 0b010:
			case 0b110:
				throw_not_implemented_yet();
				break;

			default:
				throw_internal_error();
			}

			break;
		}

		case 2:
		{
			switch (opmode)
			{
			case 0b100:
				write_and_idle<std::uint8_t>(ea_dec.result().pointer().address, res);
				break;
			
			case 0b101:
				write_and_idle<std::uint16_t>(ea_dec.result().pointer().address, res);
				break;

			case 0b110:
				throw_not_implemented_yet();

			default:
				throw_internal_error();
			}

			break;
		}

		default:
			throw_internal_error();
		}
	}

	void exec_addi()
	{
		std::cout << "exec_addi" << std::endl;

		const std::uint8_t size = ((opcode >> 6) & 0b11) + 1;
		if(size != 1 && size != 2 && size != 4)
			throw_invalid_opcode();

		switch (exec_stage++)
		{
		case 0:
			read_imm(size);
			break;

		case 1:
			decode_ea(pq.IRD & 0xFF, size);
			break;

		case 2:
		{
			auto op = ea_dec.result();

			switch (size)
			{
			case 1:
				if(op.has_pointer())
					res = add<std::uint8_t>(imm, op.pointer().value<std::uint8_t>());
				else if(op.has_data_reg())
					op.data_reg().B = add<std::uint8_t>(imm, op.data_reg().B);
				else
					throw_invalid_opcode();
				break;

			case 2:
				if(op.has_pointer())
					res = add<std::uint16_t>(imm, op.pointer().value<std::uint16_t>());
				else if(op.has_data_reg())
					op.data_reg().W = add<std::uint16_t>(imm, op.data_reg().W);
				else if(op.has_addr_reg())
					op.addr_reg().W = add<std::uint16_t>(imm, op.addr_reg().W);
				else
					throw_internal_error();
				break;

			case 4:
				throw_not_implemented_yet();

			default:
				throw_internal_error();
			}

			if(op.has_pointer())
				prefetch();
			else
				prefetch_and_idle();

			break;
		}

		case 3:
		{
			auto op = ea_dec.result();
			if(op.has_pointer())
			{
				if(size == 1)
					write_and_idle<std::uint8_t>(op.pointer().address, res);
				else if(size == 2)
					write_and_idle<std::uint16_t>(op.pointer().address, res);
				else
					throw_not_implemented_yet();
			}
			else
			{
				throw_internal_error();
			}

			break;
		}

		default:
			throw std::runtime_error("executioner::exec_addi internal error: unknown state");
		}
	}

	void exec_addq()
	{
		const std::uint8_t size = ((opcode >> 6) & 0b11) + 1;

		std::cout << "exec_addq, size: " << (int)size << std::endl;

		if(size != 1 && size != 2 && size != 4)
			throw_invalid_opcode();

		switch (exec_stage++)
		{
		case 0:
			decode_ea(pq.IRD & 0xFF, size);
			break;

		case 1:
		{
			std::uint8_t data = (opcode >> 9) & 0x7;
			if(data == 0) data = 8;

			auto op = ea_dec.result();

			switch (size)
			{
			case 1:
				if(op.has_pointer())
					res = add(data, op.pointer().value<std::uint8_t>());
				else if(op.has_data_reg())
					op.data_reg().B = add(data, op.data_reg().B);
				else
					throw_invalid_opcode();
				break;
			
			case 2:
				if(op.has_pointer())
					res = add<std::uint16_t>(data, op.pointer().value<std::uint16_t>());
				else if(op.has_data_reg())
					op.data_reg().W = add<std::uint16_t>(data, op.data_reg().W);
				else if(op.has_addr_reg())
					op.addr_reg().W = add<std::uint16_t>(data, op.addr_reg().W);
				else
					throw_internal_error();
				break;

			case 4:
				throw_not_implemented_yet();

			default:
				throw_internal_error();
			}

			if(size == 1 || (op.has_data_reg() && size == 2))
				prefetch_and_idle();
			else
				prefetch();

			break;
		}

		case 2:
		{
			auto op = ea_dec.result();
			if(op.has_pointer())
			{
				if(size == 1)
					write_and_idle<std::uint8_t>(op.pointer().address, res);
				else if(size == 2)
					write_and_idle<std::uint16_t>(op.pointer().address, res);
				else
					throw_not_implemented_yet();
			}
			else
			{
				// wait 4 cycles
			}

			break;
		}

		case 3: break;
		case 4: break;
		case 5: ex_state = IDLE; break;

		default:
			throw_internal_error();
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
	std::uint32_t res = 0;
	std::uint8_t exec_stage;

	std::uint32_t imm;
};

}

#endif // __M68K_EXECUTIONER_HPP__
