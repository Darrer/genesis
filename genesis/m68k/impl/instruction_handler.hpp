#ifndef __M68K_INSTRUCTION_HANDLER_HPP__
#define __M68K_INSTRUCTION_HANDLER_HPP__

#include "base_handler.h"
#include "ea_decoder.hpp"

#include "exception.hpp"

#include <iostream>


namespace genesis::m68k
{

#define throw_invalid_opcode() \
	throw std::runtime_error(std::string("executioner::") + __func__ + " error: invalid opcode")

class instruction_handler : public base_handler
{
private:
	enum inst_state : std::uint8_t
	{
		IDLE,
		STARTING,
		EXECUTING,
		DECODING,
	};

public:
	instruction_handler(m68k::cpu_registers& regs, m68k::bus_manager& busm, m68k::prefetch_queue& pq)
		: base_handler(regs, busm, pq), dec(busm, regs, pq)
	{
		reset();
	}

	void reset()
	{
		state = IDLE;
		opcode = 0;
		exec_stage = 0;
		res = 0;

		base_handler::reset();
	}

	bool is_idle() const
	{
		return state == IDLE && base_handler::is_idle();
	}

protected:
	void on_cycle() override
	{
		switch (state)
		{
		case IDLE:
			state = STARTING;
			[[fallthrough]];
		
		case STARTING:
			reset();
			opcode = pq.IRD;
			regs.PC += 2;
			state = EXECUTING;
			execute();
			break;

		case DECODING:
			if(dec.ready())
			{
				state = EXECUTING;
				execute();
			}
			break;

		case EXECUTING:
			execute();
			break;

		default:
			throw internal_error();
		}

		dec.cycle();
	}

	void on_idle() override
	{
		state = IDLE;
	}

private:
	void execute()
	{
		if((opcode >> 12) == 0b1101)
			exec_add();
		else if((opcode >> 8) == 0b110)
			exec_addi();
		else if((opcode >> 12) == 0b0101 && ((opcode >> 8) & 1) == 0)
			exec_addq();
		else
			throw not_implemented(std::to_string(opcode));
	}

	void exec_add()
	{
		// std::cout << "exec_add" << std::endl;

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
			auto op = dec.result();

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
					throw internal_error();
				
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
				throw not_implemented();
				break;

			default:
				throw internal_error();
			}

			break;
		}

		case 2:
		{
			switch (opmode)
			{
			case 0b100:
				write_byte_and_idle(dec.result().pointer().address, res);
				break;
			
			case 0b101:
				write_word_and_idle(dec.result().pointer().address, res);
				break;

			case 0b110:
				throw not_implemented();

			default:
				throw internal_error();
			}

			break;
		}

		default:
			throw internal_error();
		}
	}

	void exec_addi()
	{
		// std::cout << "exec_addi" << std::endl;

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
			auto op = dec.result();

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
					throw internal_error();
				break;

			case 4:
				throw not_implemented();

			default:
				throw internal_error();
			}

			if(op.has_pointer())
				prefetch();
			else
				prefetch_and_idle();

			break;
		}

		case 3:
		{
			auto op = dec.result();
			if(op.has_pointer())
			{
				if(size == 1)
					write_byte_and_idle(op.pointer().address, res);
				else if(size == 2)
					write_word_and_idle(op.pointer().address, res);
				else
					throw not_implemented();
			}
			else
			{
				throw internal_error();
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

		// std::cout << "exec_addq, size: " << (int)size << std::endl;

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

			auto op = dec.result();

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
					throw internal_error();
				break;

			case 4:
				throw not_implemented();

			default:
				throw internal_error();
			}

			if(op.has_pointer() || (op.has_addr_reg() && size == 2))
				prefetch();
			else
				prefetch_and_idle();

			break;
		}

		case 2:
		{
			auto op = dec.result();
			if(op.has_pointer())
			{
				if(size == 1)
					write_byte_and_idle(op.pointer().address, res);
				else if(size == 2)
					write_word_and_idle(op.pointer().address, res);
				else
					throw not_implemented();
			}
			else
			{
				// wait 4 cycles
			}

			break;
		}

		case 3: break;
		case 4: break;
		case 5: state = IDLE; break;

		default:
			throw internal_error();
		}
	}

private:
	// TODO: move away
	template<class T>
	T add(T a, T b)
	{
		return a + b;
	}

private:
	void decode_ea(std::uint8_t ea, std::uint8_t size)
	{
		dec.decode(ea, size);
		if(dec.ready())
		{
			// immediate decoding
			execute();
		}
		else
		{
			// wait till decoding is over
			state = DECODING;
		}
	}

private:
	m68k::ea_decoder dec;

	std::uint16_t opcode = 0;
	std::uint32_t res = 0;
	std::uint8_t exec_stage;

	std::uint8_t state;
};

}

#endif // __M68K_INSTRUCTION_HANDLER_HPP__
